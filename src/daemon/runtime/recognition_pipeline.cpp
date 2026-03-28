#include "daemon/runtime/recognition_pipeline.h"

#include "common/dbus/dbus_interface.h"

namespace vinput::daemon::runtime {

RecognitionPipeline::RecognitionPipeline(PostProcessor *post_processor)
    : post_processor_(post_processor) {}

RecognitionPipelineResult RecognitionPipeline::Process(
    const RecognitionOrder &order, const CoreConfig &settings,
    const std::function<void()> &on_enter_postprocessing) const {
  RecognitionPipelineResult output;
  if (!post_processor_) {
    output.errors.push_back(vinput::dbus::MakeRawError(
        "Recognition pipeline is not initialized."));
    return output;
  }

  if (order.recognized_text.empty()) {
    return output;
  }

  vinput::scene::Config scene_config;
  scene_config.activeSceneId = settings.scenes.activeScene;
  scene_config.scenes = settings.scenes.definitions;

  if (order.is_command) {
    const auto *cmd_scene = FindCommandScene(settings);
    if (cmd_scene && cmd_scene->candidate_count > 0 &&
        !cmd_scene->provider_id.empty() && on_enter_postprocessing) {
      on_enter_postprocessing();
    }

    vinput::scene::Definition fallback_cmd;
    fallback_cmd.id = std::string(vinput::scene::kCommandSceneId);
    fallback_cmd.builtin = true;
    const auto &command_scene = cmd_scene ? *cmd_scene : fallback_cmd;
    std::string llm_error;
    output.payload = post_processor_->ProcessCommand(
        order.recognized_text, order.selected_text, command_scene, settings,
        &llm_error);
    if (!llm_error.empty()) {
      output.errors.push_back(vinput::dbus::ClassifyErrorText(llm_error));
    }
    return output;
  }

  const auto &scene = vinput::scene::Resolve(scene_config, order.scene_id);
  if (scene.candidate_count > 0 && !scene.provider_id.empty() &&
      !scene.prompt.empty() && on_enter_postprocessing) {
    on_enter_postprocessing();
  }

  std::string llm_error;
  output.payload = post_processor_->Process(order.recognized_text, scene,
                                            settings, &llm_error);
  if (!llm_error.empty()) {
    output.errors.push_back(vinput::dbus::ClassifyErrorText(llm_error));
  }
  return output;
}

}  // namespace vinput::daemon::runtime
