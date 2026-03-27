#include <memory>

#include <CLI/CLI.hpp>

#include "cli/command_recording.h"
#include "cli/control/action.h"
#include "common/i18n.h"

namespace vinput::cli::control {

void RegisterRecordingCommands(CLI::App &app, CliAction *action) {
  auto *recording = app.add_subcommand("recording", _("Control voice recording"));
  recording->alias("rec");
  recording->require_subcommand(1);

  auto *start = recording->add_subcommand("start", _("Start recording"));
  start->callback([action]() {
    *action = [](Formatter &fmt, const CliContext &ctx) {
      return RunRecordingStart(fmt, ctx);
    };
  });

  auto stopSceneId = std::make_shared<std::string>();
  auto *stop =
      recording->add_subcommand("stop", _("Stop recording and recognize"));
  stop->add_option("-s,--scene", *stopSceneId,
                   _("Scene id (default: active scene)"));
  stop->callback([action, stopSceneId]() {
    *action = [stopSceneId](Formatter &fmt, const CliContext &ctx) {
      return RunRecordingStop(*stopSceneId, fmt, ctx);
    };
  });

  auto toggleSceneId = std::make_shared<std::string>();
  auto *toggle =
      recording->add_subcommand("toggle", _("Toggle recording start/stop"));
  toggle->add_option("-s,--scene", *toggleSceneId,
                     _("Scene id (default: active scene)"));
  toggle->callback([action, toggleSceneId]() {
    *action = [toggleSceneId](Formatter &fmt, const CliContext &ctx) {
      return RunRecordingToggle(*toggleSceneId, fmt, ctx);
    };
  });
}

}  // namespace vinput::cli::control
