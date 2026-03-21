#include "vinput.h"
#include "common/dbus_interface.h"
#include "common/i18n.h"
#include "common/recognition_result.h"

#include "notifications_public.h"
#include <dbus_public.h>
#include <fcitx-utils/dbus/matchrule.h>
#include <fcitx-utils/dbus/message.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>

#include <cstdio>
#include <string>

using namespace vinput::dbus;

namespace {

constexpr const char *kSystemdBusName = "org.freedesktop.systemd1";
constexpr const char *kSystemdPath = "/org/freedesktop/systemd1";
constexpr const char *kSystemdManagerInterface =
    "org.freedesktop.systemd1.Manager";
constexpr const char *kSystemdRestartUnit = "RestartUnit";
constexpr uint64_t kSystemdCallTimeoutUsec = 5 * 1000 * 1000;
constexpr const char *kDaemonUnitName = "vinput-daemon.service";
constexpr const char *kReplaceMode = "replace";
constexpr uint64_t kDaemonCallTimeoutUsec = 5 * 1000 * 1000;

std::string RecordingPreeditText() { return _("... Recording ..."); }

std::string CommandingPreeditText() { return _("... Commanding ..."); }

std::string InferringPreeditText() { return _("... Recognizing ..."); }

std::string PostprocessingPreeditText() { return _("... Postprocessing ..."); }

} // namespace

void VinputEngine::setupDBusWatcher() {
  if (!bus_)
    return;

  fcitx::dbus::MatchRule result_rule(kBusName, kObjectPath, kInterface,
                                     kSignalRecognitionResult);

  result_slot_ = bus_->addMatch(result_rule, [this](fcitx::dbus::Message &msg) {
    onRecognitionResult(msg);
    return true;
  });

  fcitx::dbus::MatchRule status_rule(kBusName, kObjectPath, kInterface,
                                     kSignalStatusChanged);

  status_slot_ = bus_->addMatch(status_rule, [this](fcitx::dbus::Message &msg) {
    onStatusChanged(msg);
    return true;
  });

  fcitx::dbus::MatchRule error_rule(kBusName, kObjectPath, kInterface,
                                    kSignalDaemonError);

  error_slot_ =
      bus_->addMatch(error_rule, [this](fcitx::dbus::Message &msg) {
        onDaemonError(msg);
        return true;
      });
}

void VinputEngine::callStartRecording() {
  if (!bus_)
    return;
  auto msg = bus_->createMethodCall(kBusName, kObjectPath, kInterface,
                                    kMethodStartRecording);
  auto reply = msg.call(kDaemonCallTimeoutUsec);
  if (!reply || reply.isError()) {
    fprintf(stderr, "vinput: StartRecording rejected by daemon\n");
    auto *ic = session_ ? session_->ic : nullptr;
    session_.reset();
    if (ic)
      clearPreedit(ic);
  }
}

void VinputEngine::callStartCommandRecording(const std::string &selected_text) {
  if (!bus_)
    return;
  auto msg = bus_->createMethodCall(kBusName, kObjectPath, kInterface,
                                    kMethodStartCommandRecording);
  msg << selected_text;
  auto reply = msg.call(kDaemonCallTimeoutUsec);
  if (!reply || reply.isError()) {
    fprintf(stderr, "vinput: StartCommandRecording rejected by daemon\n");
    auto *ic = session_ ? session_->ic : nullptr;
    session_.reset();
    if (ic)
      clearPreedit(ic);
  }
}

void VinputEngine::callStopRecording(const std::string &scene_id) {
  if (!bus_)
    return;
  auto msg = bus_->createMethodCall(kBusName, kObjectPath, kInterface,
                                    kMethodStopRecording);
  msg << scene_id;
  auto reply = msg.call(kDaemonCallTimeoutUsec);
  if (!reply || reply.isError()) {
    fprintf(stderr, "vinput: StopRecording rejected by daemon\n");
    auto *ic = session_ ? session_->ic : nullptr;
    session_.reset();
    if (ic)
      clearPreedit(ic);
  }
}

void VinputEngine::restartDaemon() {
  if (!bus_) {
    fprintf(
        stderr,
        "vinput: cannot restart vinput-daemon because DBus is unavailable\n");
    return;
  }

  auto msg =
      bus_->createMethodCall(kSystemdBusName, kSystemdPath,
                             kSystemdManagerInterface, kSystemdRestartUnit);
  msg << kDaemonUnitName << kReplaceMode;

  auto reply = msg.call(kSystemdCallTimeoutUsec);
  if (!reply) {
    fprintf(stderr,
            "vinput: failed to restart vinput-daemon via systemd user bus\n");
    return;
  }

  if (reply.isError()) {
    fprintf(stderr, "vinput: systemd restart failed: %s: %s\n",
            reply.errorName().c_str(), reply.errorMessage().c_str());
  }
}

void VinputEngine::onRecognitionResult(fcitx::dbus::Message &msg) {
  std::string payload_text;
  msg >> payload_text;

  const bool is_command = session_ && session_->command_mode;
  auto *ic = session_ ? session_->ic : nullptr;
  session_.reset();

  if (!ic) {
    return;
  }

  hideResultMenu();

  const auto payload = vinput::result::Parse(payload_text);
  clearPreedit(ic);

  if (payload.commitText.empty()) {
    return;
  }

  int llm_count = 0;
  for (const auto &c : payload.candidates) {
    if (c.source == vinput::result::kSourceLlm) ++llm_count;
  }
  if (llm_count > 1) {
    // Save command mode for result menu interaction
    result_is_command_ = is_command;
    showResultMenu(ic, payload);
    return;
  }

  if (is_command) {
    auto &surrounding = ic->surroundingText();
    if (surrounding.isValid() && surrounding.cursor() != surrounding.anchor()) {
      int cursor = surrounding.cursor();
      int anchor = surrounding.anchor();
      int from = std::min(cursor, anchor);
      int len = std::abs(cursor - anchor);
      ic->deleteSurroundingText(from - cursor, len);
    }
  }

  ic->commitString(payload.commitText);
}

void VinputEngine::onStatusChanged(fcitx::dbus::Message &msg) {
  std::string status;
  msg >> status;

  if (!session_)
    return;

  auto *ic = session_->ic;
  if (!ic)
    return;

  if (status == kStatusRecording) {
    updatePreedit(ic, session_->command_mode ? CommandingPreeditText() : RecordingPreeditText());
  } else if (status == kStatusInferring) {
    updatePreedit(ic, InferringPreeditText());
  } else if (status == kStatusPostprocessing) {
    updatePreedit(ic, PostprocessingPreeditText());
  } else {
    session_.reset();
    clearPreedit(ic);
  }
}

void VinputEngine::onDaemonError(fcitx::dbus::Message &msg) {
  std::string error_message;
  msg >> error_message;

  if (error_message.empty()) {
    return;
  }

  auto *ic = session_ ? session_->ic : nullptr;
  session_.reset();
  hideResultMenu();
  if (ic) {
    clearPreedit(ic);
  }

  notifyError(error_message);
}

void VinputEngine::notifyError(const std::string &message) {
  if (message.empty()) {
    return;
  }

  auto *notifications =
      instance_->addonManager().addon("notifications", true);
  if (notifications) {
    notifications->call<fcitx::INotifications::sendNotification>(
        "fcitx5-vinput", 0, "dialog-error",
        _("Voice Input"), message, std::vector<std::string>{},
        5000, fcitx::NotificationActionCallback{},
        fcitx::NotificationClosedCallback{});
  } else {
    fprintf(stderr, "vinput: %s\n", message.c_str());
  }
}

void VinputEngine::updatePreedit(fcitx::InputContext *ic,
                                 const std::string &text) {
  if (!ic)
    return;
  fcitx::Text preedit;
  preedit.append(text);
  ic->inputPanel().setPreedit(preedit);
  ic->inputPanel().setClientPreedit(preedit);
  ic->updatePreedit();
  ic->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
}

void VinputEngine::clearPreedit(fcitx::InputContext *ic) {
  if (!ic)
    return;
  fcitx::Text empty;
  ic->inputPanel().setPreedit(empty);
  ic->inputPanel().setClientPreedit(empty);
  ic->updatePreedit();
  ic->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
}
