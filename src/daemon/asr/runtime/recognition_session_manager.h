#pragma once

#include "common/config/core_config.h"
#include "daemon/asr/runtime/recognition_contract.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace vinput::daemon::asr {

struct RecognitionRunResult {
  bool available = false;
  bool ok = true;
  std::string text;
  std::string error;
};

class RecognitionSessionManager {
public:
  explicit RecognitionSessionManager(bool disable_asr_by_flag);
  ~RecognitionSessionManager();

  bool Initialize(const CoreConfig &settings, std::string *disabled_reason);
  std::unique_ptr<RecognitionSession>
  CreateSession(const CoreConfig &settings, BackendDescriptor *descriptor,
                std::string *error);
  static RecognitionRunResult
  ConsumeEvents(std::unique_ptr<RecognitionSession> *session, bool cancel,
                std::string *error);
  RecognitionRunResult Recognize(const CoreConfig &settings,
                                 const std::vector<int16_t> &pcm_data);
  void Shutdown();

private:
  bool EnsureBackendReady(const CoreConfig &settings, std::string *error);
  void ResetBackend();

  bool disable_asr_by_flag_ = false;
  std::unique_ptr<AsrBackend> backend_;
  std::string backend_signature_;
};

}  // namespace vinput::daemon::asr
