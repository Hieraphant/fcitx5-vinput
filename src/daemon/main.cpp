#include "daemon/audio/audio_capture.h"
#include "common/llm/adaptor_manager.h"
#include "common/config/core_config.h"
#include "common/dbus/dbus_interface.h"
#include "common/i18n.h"
#include "common/utils/process_utils.h"
#include "common/asr/recognition_result.h"
#include "common/utils/string_utils.h"
#include "daemon/asr/runtime/recognition_session_manager.h"
#include "daemon/runtime/daemon_runtime_controller.h"
#include "daemon/runtime/dbus_service.h"
#include "daemon/runtime/recognition_pipeline.h"
#include "daemon/postprocess/post_processor.h"

#include <poll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

static std::atomic<bool> g_running{true};

static void signal_handler(int sig) {
  (void)sig;
  g_running = false;
}

namespace {

std::string ReadAvailableText(int fd) {
  std::string text;
  char buffer[4096];
  while (true) {
    const ssize_t n = read(fd, buffer, sizeof(buffer));
    if (n > 0) {
      text.append(buffer, static_cast<std::size_t>(n));
      continue;
    }
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      break;
    }
    break;
  }
  return text;
}

class AdaptorSupervisor {
public:
  explicit AdaptorSupervisor(DbusService *dbus) : dbus_(dbus) {}

  ~AdaptorSupervisor() {
    Shutdown();
    if (wake_fd_ >= 0) {
      close(wake_fd_);
    }
  }

  bool Start(std::string *error) {
    wake_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wake_fd_ < 0) {
      if (error) {
        *error = std::string("failed to create adaptor wake fd: ") +
                 strerror(errno);
      }
      return false;
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      running_ = true;
      stop_requested_ = false;
    }

    try {
      thread_ = std::thread([this]() { Run(); });
    } catch (const std::exception &e) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        stop_requested_ = true;
      }
      if (error) {
        *error =
            std::string("failed to start adaptor supervisor thread: ") + e.what();
      }
      close(wake_fd_);
      wake_fd_ = -1;
      return false;
    }

    if (error) {
      error->clear();
    }
    return true;
  }

  void Shutdown() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!running_) {
        return;
      }
      running_ = false;
      stop_requested_ = true;
    }
    Wake();
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  DbusService::MethodResult StartAdaptor(const std::string &adaptor_id) {
    return SubmitRequest(Request::Type::Start, adaptor_id);
  }

  DbusService::MethodResult StopAdaptor(const std::string &adaptor_id) {
    return SubmitRequest(Request::Type::Stop, adaptor_id);
  }

private:
  struct ManagedAdaptor {
    std::string id;
    pid_t pid = -1;
    int stderr_fd = -1;
    std::string stderr_buffer;
  };

  struct Request {
    enum class Type { Start, Stop };

    Type type = Type::Start;
    std::string adaptor_id;
    DbusService::MethodResult result;
    bool done = false;
    std::condition_variable cv;
  };

  DbusService::MethodResult SubmitRequest(Request::Type type,
                                          const std::string &adaptor_id) {
    auto request = std::make_shared<Request>();
    request->type = type;
    request->adaptor_id = adaptor_id;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!running_ || stop_requested_) {
        return DbusService::MethodResult::Failure(
            "adaptor supervisor is not running");
      }
      pending_requests_.push_back(request);
    }

    Wake();

    std::unique_lock<std::mutex> lock(mutex_);
    request->cv.wait(lock, [&]() { return request->done; });
    return request->result;
  }

  void Wake() {
    if (wake_fd_ < 0) {
      return;
    }
    uint64_t value = 1;
    (void)write(wake_fd_, &value, sizeof(value));
  }

  void DrainWakeFd() {
    if (wake_fd_ < 0) {
      return;
    }
    uint64_t value = 0;
    while (read(wake_fd_, &value, sizeof(value)) == sizeof(value)) {
    }
  }

  void EmitNotification(std::string_view text) {
    const std::string notification = vinput::str::TrimAsciiWhitespace(text);
    if (notification.empty()) {
      return;
    }
    dbus_->EmitError(vinput::dbus::MakeRawError(notification));
  }

  void FlushAdaptorBuffer(ManagedAdaptor &adaptor, bool flush_partial) {
    size_t start = 0;
    while (true) {
      const size_t end = adaptor.stderr_buffer.find('\n', start);
      if (end == std::string::npos) {
        break;
      }
      std::string line = adaptor.stderr_buffer.substr(start, end - start);
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (!line.empty()) {
        fprintf(stderr, "vinput-daemon: adaptor[%s] stderr: %s\n",
                adaptor.id.c_str(), line.c_str());
        EmitNotification(line);
      }
      start = end + 1;
    }

    adaptor.stderr_buffer.erase(0, start);
    if (flush_partial) {
      std::string line = vinput::str::TrimAsciiWhitespace(adaptor.stderr_buffer);
      if (!line.empty()) {
        fprintf(stderr, "vinput-daemon: adaptor[%s] stderr: %s\n",
                adaptor.id.c_str(), line.c_str());
        EmitNotification(line);
      }
      adaptor.stderr_buffer.clear();
    }
  }

  DbusService::MethodResult HandleStartRequest(const std::string &adaptor_id) {
    auto runtime_settings = LoadCoreConfig();
    NormalizeCoreConfig(&runtime_settings);
    const auto *adaptor = ResolveLlmAdaptor(runtime_settings, adaptor_id);
    if (!adaptor) {
      return DbusService::MethodResult::Failure("adaptor not found");
    }

    if (adaptors_.find(adaptor_id) != adaptors_.end() ||
        vinput::adaptor::IsRunning(adaptor_id)) {
      return DbusService::MethodResult::Failure("adaptor is already running");
    }

    if (adaptor->command.empty()) {
      return DbusService::MethodResult::Failure(
          "adaptor command is not configured");
    }

    std::string error;
    const auto spec = vinput::adaptor::BuildCommandSpec(*adaptor);
    const auto working_dir = vinput::adaptor::ResolveWorkingDir(*adaptor);
    vinput::process::SpawnedProcess process;
    if (!vinput::process::SpawnForMonitoring(spec, working_dir,
                                             &process, &error)) {
      return DbusService::MethodResult::Failure(
          error.empty() ? "failed to start adaptor" : error);
    }

    usleep(250000);
    int status = 0;
    if (waitpid(process.pid, &status, WNOHANG) == process.pid) {
      std::string stderr_text = ReadAvailableText(process.stderr_fd);
      close(process.stderr_fd);
      process.stderr_fd = -1;
      stderr_text = vinput::str::TrimAsciiWhitespace(stderr_text);
      if (stderr_text.empty()) {
        stderr_text = "adaptor exited immediately";
      }
      return DbusService::MethodResult::Failure(stderr_text);
    }

    if (!vinput::adaptor::WritePidFile(adaptor_id, process.pid, &error)) {
      kill(process.pid, SIGTERM);
      (void)waitpid(process.pid, nullptr, 0);
      close(process.stderr_fd);
      process.stderr_fd = -1;
      return DbusService::MethodResult::Failure(
          error.empty() ? "failed to persist adaptor pid" : error);
    }

    adaptors_.emplace(adaptor_id, ManagedAdaptor{
                                     .id = adaptor_id,
                                     .pid = process.pid,
                                     .stderr_fd = process.stderr_fd,
                                     .stderr_buffer = {},
                                 });
    fprintf(stderr, "vinput-daemon: adaptor started id=%s pid=%d\n",
            adaptor_id.c_str(), static_cast<int>(process.pid));
    return DbusService::MethodResult::Success();
  }

  DbusService::MethodResult HandleStopRequest(const std::string &adaptor_id) {
    auto runtime_settings = LoadCoreConfig();
    NormalizeCoreConfig(&runtime_settings);
    if (!ResolveLlmAdaptor(runtime_settings, adaptor_id)) {
      return DbusService::MethodResult::Failure("adaptor not found");
    }

    auto it = adaptors_.find(adaptor_id);
    pid_t tracked_pid = -1;
    if (it != adaptors_.end()) {
      tracked_pid = it->second.pid;
      if (it->second.stderr_fd >= 0) {
        close(it->second.stderr_fd);
        it->second.stderr_fd = -1;
      }
      adaptors_.erase(it);
    }

    std::string error;
    if (!vinput::adaptor::Stop(adaptor_id, &error)) {
      return DbusService::MethodResult::Failure(
          error.empty() ? "failed to stop adaptor" : error);
    }

    if (tracked_pid > 0) {
      (void)waitpid(tracked_pid, nullptr, 0);
    }

    fprintf(stderr, "vinput-daemon: adaptor stopped id=%s\n", adaptor_id.c_str());
    return DbusService::MethodResult::Success();
  }

  void ProcessPendingRequests() {
    std::vector<std::shared_ptr<Request>> requests;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      requests.swap(pending_requests_);
    }

    for (const auto &request : requests) {
      DbusService::MethodResult result;
      if (request->type == Request::Type::Start) {
        result = HandleStartRequest(request->adaptor_id);
      } else {
        result = HandleStopRequest(request->adaptor_id);
      }

      {
        std::lock_guard<std::mutex> lock(mutex_);
        request->result = std::move(result);
        request->done = true;
      }
      request->cv.notify_one();
    }
  }

  void ReapExitedAdaptors() {
    std::vector<std::string> exited_ids;
    for (auto &[id, adaptor] : adaptors_) {
      int status = 0;
      if (waitpid(adaptor.pid, &status, WNOHANG) != adaptor.pid) {
        continue;
      }

      if (adaptor.stderr_fd >= 0) {
        adaptor.stderr_buffer += ReadAvailableText(adaptor.stderr_fd);
      }
      FlushAdaptorBuffer(adaptor, true);
      if (adaptor.stderr_fd >= 0) {
        close(adaptor.stderr_fd);
        adaptor.stderr_fd = -1;
      }

      fprintf(stderr, "vinput-daemon: adaptor exited id=%s code=%d\n",
              adaptor.id.c_str(),
              WIFEXITED(status)
                  ? WEXITSTATUS(status)
                  : (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1));
      exited_ids.push_back(id);
    }

    for (const auto &id : exited_ids) {
      vinput::adaptor::RemovePidFile(id);
      adaptors_.erase(id);
    }
  }

  void PollAdaptorsOnce() {
    std::vector<pollfd> fds;
    fds.reserve(1 + adaptors_.size());
    fds.push_back({.fd = wake_fd_, .events = POLLIN, .revents = 0});

    std::vector<std::string> adaptor_ids;
    adaptor_ids.reserve(adaptors_.size());
    for (const auto &[id, adaptor] : adaptors_) {
      if (adaptor.stderr_fd < 0) {
        continue;
      }
      fds.push_back({.fd = adaptor.stderr_fd,
                     .events = static_cast<short>(POLLIN | POLLHUP | POLLERR),
                     .revents = 0});
      adaptor_ids.push_back(id);
    }

    const int ret = poll(fds.data(), static_cast<nfds_t>(fds.size()), 1000);
    if (ret < 0) {
      if (errno != EINTR) {
        fprintf(stderr, "vinput-daemon: adaptor poll error: %s\n",
                strerror(errno));
      }
      return;
    }

    if (ret > 0 && (fds[0].revents & POLLIN)) {
      DrainWakeFd();
      ProcessPendingRequests();
    }

    for (size_t i = 0; i < adaptor_ids.size(); ++i) {
      auto it = adaptors_.find(adaptor_ids[i]);
      if (it == adaptors_.end()) {
        continue;
      }

      ManagedAdaptor &adaptor = it->second;
      const pollfd &fd = fds[i + 1];
      if ((fd.revents & (POLLIN | POLLHUP | POLLERR)) == 0 ||
          adaptor.stderr_fd < 0) {
        continue;
      }

      adaptor.stderr_buffer += ReadAvailableText(adaptor.stderr_fd);
      const bool closing_stderr = (fd.revents & (POLLHUP | POLLERR)) != 0;
      FlushAdaptorBuffer(adaptor, closing_stderr);
      if (closing_stderr) {
        close(adaptor.stderr_fd);
        adaptor.stderr_fd = -1;
      }
    }

    ReapExitedAdaptors();
  }

  void ShutdownAdaptors() {
    for (auto &[id, adaptor] : adaptors_) {
      if (adaptor.stderr_fd >= 0) {
        close(adaptor.stderr_fd);
        adaptor.stderr_fd = -1;
      }
      if (adaptor.pid > 0) {
        kill(adaptor.pid, SIGTERM);
        (void)waitpid(adaptor.pid, nullptr, 0);
      }
      vinput::adaptor::RemovePidFile(id);
    }
    adaptors_.clear();
  }

  void Run() {
    while (true) {
      ProcessPendingRequests();
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_requested_) {
          break;
        }
      }
      PollAdaptorsOnce();
    }

    ProcessPendingRequests();
    ShutdownAdaptors();
  }

  DbusService *dbus_ = nullptr;
  int wake_fd_ = -1;
  std::thread thread_;
  std::mutex mutex_;
  bool running_ = false;
  bool stop_requested_ = false;
  std::vector<std::shared_ptr<Request>> pending_requests_;
  std::map<std::string, ManagedAdaptor> adaptors_;
};

}  // namespace

int main(int argc, char *argv[]) {
  vinput::i18n::Init();
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  bool disable_asr = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--no-asr") == 0) {
      disable_asr = true;
    }
  }

  auto startup_settings = LoadCoreConfig();
  NormalizeCoreConfig(&startup_settings);
  vinput::daemon::asr::RecognitionSessionManager recognition_manager(disable_asr);

  std::string asr_disabled_reason;
  if (!recognition_manager.Initialize(startup_settings, &asr_disabled_reason)) {
    fprintf(stderr, "vinput-daemon: running with ASR disabled");
    if (!asr_disabled_reason.empty()) {
      fprintf(stderr, " (%s)", asr_disabled_reason.c_str());
    }
    fprintf(stderr, "\n");
  }

  AudioCapture capture;
  std::string capture_error;
  if (!capture.Start(&capture_error)) {
    if (capture_error.empty()) {
      capture_error = "audio capture start failed";
    }
    fprintf(stderr, "vinput-daemon: %s\n", capture_error.c_str());
    return 1;
  }

  DbusService dbus;
  PostProcessor post_processor;

  using vinput::dbus::Status;
  using vinput::dbus::StatusToString;

  // --- Single-slot state (all protected by state_mutex) ---
  AdaptorSupervisor adaptor_supervisor(&dbus);
  vinput::daemon::runtime::RecognitionPipeline recognition_pipeline(
      &post_processor);
  vinput::daemon::runtime::DaemonRuntimeController runtime_controller(
      &capture, &dbus, &recognition_manager, &recognition_pipeline);

  dbus.SetStartHandler([&]() {
    return runtime_controller.StartRecording();
  });

  dbus.SetStartCommandHandler([&](const std::string &selected_text) {
    return runtime_controller.StartCommandRecording(selected_text);
  });

  dbus.SetStopHandler(
      [&](const std::string &scene_id) -> DbusService::MethodResult {
    return runtime_controller.StopRecording(scene_id);
  });

  dbus.SetStatusHandler([&]() -> std::string { return runtime_controller.GetStatus(); });

  dbus.SetStartAdaptorHandler(
      [&](const std::string &adaptor_id) -> DbusService::MethodResult {
    return adaptor_supervisor.StartAdaptor(adaptor_id);
  });

  dbus.SetStopAdaptorHandler(
      [&](const std::string &adaptor_id) -> DbusService::MethodResult {
    return adaptor_supervisor.StopAdaptor(adaptor_id);
  });

  std::string dbus_error;
  if (!dbus.Start(&dbus_error)) {
    if (dbus_error.empty()) {
      dbus_error = "DBus service start failed";
    }
    fprintf(stderr, "vinput-daemon: %s\n", dbus_error.c_str());
    return 1;
  }

  std::string adaptor_error;
  if (!adaptor_supervisor.Start(&adaptor_error)) {
    fprintf(stderr, "vinput-daemon: %s\n", adaptor_error.c_str());
    return 1;
  }

  runtime_controller.StartWorker();

  fprintf(stderr, "vinput-daemon: running\n");

  int dbus_fd = dbus.GetFd();
  int notify_fd = dbus.GetNotifyFd();
  while (g_running) {
    pollfd fds[2] = {
        {.fd = dbus_fd, .events = POLLIN, .revents = 0},
        {.fd = notify_fd, .events = POLLIN, .revents = 0},
    };

    int ret = poll(fds, 2, 1000);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      fprintf(stderr, "vinput-daemon: poll error: %s\n", strerror(errno));
      break;
    }

    if (ret > 0) {
      if (fds[1].revents & POLLIN) {
        dbus.FlushEmitQueue();
      }
      if (fds[0].revents & POLLIN) {
        while (dbus.ProcessOnce()) {
          // process all pending messages
        }
      }
    }
  }

  fprintf(stderr, "vinput-daemon: shutting down\n");
  runtime_controller.Shutdown();
  recognition_manager.Shutdown();
  adaptor_supervisor.Shutdown();
  return 0;
}
