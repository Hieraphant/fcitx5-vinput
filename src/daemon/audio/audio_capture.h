#pragma once

#include <pipewire/pipewire.h>
#include <pipewire/thread-loop.h>
#include <spa/param/audio/format-utils.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <span>
#include <string>
#include <vector>

class AudioCapture {
public:
  AudioCapture();
  ~AudioCapture();

  using ChunkCallback = std::function<void(std::span<const int16_t>)>;

  bool Start(std::string *error = nullptr);
  std::vector<int16_t> StopAndGetBuffer();
  void Stop();
  bool BeginRecording(std::string *error = nullptr);
  void EndRecording();
  bool IsRecording() const;
  void SetTargetObject(std::string target_object);
  void SetChunkCallback(ChunkCallback callback);

private:
  static void onProcess(void *userdata);
  static void onParamChanged(void *userdata, uint32_t id,
                             const struct spa_pod *param);
  bool CreateStream(std::string *error = nullptr);
  void DestroyStream();
  void processCallback();

  struct pw_thread_loop *loop_ = nullptr;
  struct pw_stream *stream_ = nullptr;
  struct pw_stream_events stream_events_{};
  std::atomic<bool> recording_{false};
  std::mutex buffer_mutex_;
  std::mutex callback_mutex_;
  std::mutex target_mutex_;
  std::vector<int16_t> pcm_buffer_;
  std::string target_object_;
  ChunkCallback chunk_callback_;
};
