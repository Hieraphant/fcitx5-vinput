#include "daemon/asr/backends/sherpa_offline_backend.h"

#include "common/i18n.h"
#include "common/utils/string_utils.h"

#include <utility>

namespace vinput::daemon::asr {

namespace {

class SherpaOfflineSession : public RecognitionSession {
public:
  SherpaOfflineSession(ModelInfo model_info, AsrConfig asr_config,
                       std::string provider_id)
      : model_info_(std::move(model_info)),
        asr_config_(std::move(asr_config)),
        provider_id_(std::move(provider_id)) {}

  bool PushAudio(std::span<const int16_t> pcm, std::string *error) override {
    if (finished_) {
      if (error) {
        *error = "Recognition session already finished.";
      }
      return false;
    }
    pcm_.insert(pcm_.end(), pcm.begin(), pcm.end());
    if (error) {
      error->clear();
    }
    return true;
  }

  bool Finish(std::string *error) override {
    if (finished_) {
      if (error) {
        error->clear();
      }
      return true;
    }

    finished_ = true;
    std::string engine_error;
    if (!engine_.Init(model_info_, asr_config_, &engine_error)) {
      events_.push_back(
          {RecognitionEventKind::Error, {}, std::move(engine_error)});
      events_.push_back({RecognitionEventKind::Completed, {}, {}});
      if (error) {
        *error = events_.front().error;
      }
      return false;
    }

    std::string text = engine_.Infer(pcm_);
    if (!text.empty()) {
      events_.push_back({RecognitionEventKind::FinalText, std::move(text), {}});
    }
    events_.push_back({RecognitionEventKind::Completed, {}, {}});
    if (error) {
      error->clear();
    }
    return true;
  }

  void Cancel() override {
    finished_ = true;
    pcm_.clear();
    events_.clear();
    events_.push_back({RecognitionEventKind::Completed, {}, {}});
  }

  std::vector<RecognitionEvent> PollEvents() override {
    auto events = std::move(events_);
    events_.clear();
    return events;
  }

private:
  ModelInfo model_info_;
  AsrConfig asr_config_;
  std::string provider_id_;
  AsrEngine engine_;
  bool finished_ = false;
  std::vector<int16_t> pcm_;
  std::vector<RecognitionEvent> events_;
};

class SherpaOfflineBackend : public AsrBackend {
public:
  SherpaOfflineBackend(ModelInfo model_info, AsrConfig asr_config,
                       std::string provider_id, std::string model_id)
      : model_info_(std::move(model_info)),
        asr_config_(std::move(asr_config)),
        provider_id_(std::move(provider_id)),
        model_id_(std::move(model_id)) {}

  BackendDescriptor Describe() const override {
    BackendDescriptor descriptor;
    descriptor.provider_id = provider_id_;
    descriptor.provider_type = vinput::asr::kLocalProviderType;
    descriptor.backend_id = "sherpa-offline";
    descriptor.capabilities.audio_delivery_mode =
        AudioDeliveryMode::Buffered;
    descriptor.capabilities.supports_hotwords = model_info_.supports_hotwords;
    return descriptor;
  }

  std::unique_ptr<RecognitionSession>
  CreateSession(std::string *error) override {
    if (error) {
      error->clear();
    }
    return std::make_unique<SherpaOfflineSession>(model_info_, asr_config_,
                                                  provider_id_);
  }

private:
  ModelInfo model_info_;
  AsrConfig asr_config_;
  std::string provider_id_;
  std::string model_id_;
};

}  // namespace

std::unique_ptr<AsrBackend>
CreateSherpaOfflineBackend(const CoreConfig &config,
                           const LocalAsrProvider &provider,
                           std::string *error) {
  if (provider.model.empty()) {
    if (error) {
      *error = vinput::str::FmtStr(
          _("Local ASR model configuration is missing for provider '%s'."),
          provider.id);
    }
    return nullptr;
  }

  ModelManager model_mgr(ResolveModelBaseDir(config).string(), provider.model);
  std::string model_error;
  if (!model_mgr.EnsureModels(&model_error)) {
    if (error) {
      *error = "Local ASR model check failed for provider '" + provider.id + "'";
      if (!model_error.empty()) {
        *error += ": " + model_error;
      } else {
        *error += ".";
      }
    }
    return nullptr;
  }

  AsrConfig asr_config;
  const ModelInfo model_info = model_mgr.GetModelInfo();
  asr_config.language = model_info.RuntimeLanguageHint();
  asr_config.hotwords_file = provider.hotwordsFile;
  asr_config.normalize_audio = config.asr.normalizeAudio;
  asr_config.input_gain = static_cast<float>(config.asr.inputGain);
  asr_config.vad_enabled = config.asr.vad.enabled;
  asr_config.vad_model_path = VINPUT_VAD_MODEL_PATH;

  if (error) {
    error->clear();
  }
  return std::make_unique<SherpaOfflineBackend>(
      model_info, std::move(asr_config), provider.id, provider.model);
}

}  // namespace vinput::daemon::asr
