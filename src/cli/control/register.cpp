#include "cli/control/register.h"

#include <CLI/CLI.hpp>

#include "cli/control/action.h"

namespace vinput::cli::control {

void RegisterRecordingCommands(CLI::App &app, CliAction *action);

void RegisterControlCli(CLI::App &app, CliAction *action) {
  RegisterRecordingCommands(app, action);
}

}  // namespace vinput::cli::control
