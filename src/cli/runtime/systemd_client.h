#pragma once

#include "common/dbus/error_info.h"

#include <string>

namespace vinput::cli {

struct DaemonControlResult {
  int exit_code = -1;
  std::string failure_message;
  vinput::dbus::ErrorInfo notification;

  bool ok() const { return exit_code == 0; }
};

int SystemctlStart();
int SystemctlStop();
int SystemctlRestart();
int JournalctlLogs(bool follow, int lines);
std::string JournalctlLogsText(int lines);
DaemonControlResult SystemctlStartWithDiagnostics();
DaemonControlResult SystemctlRestartWithDiagnostics();
bool NotifyDaemonFailure(const vinput::dbus::ErrorInfo &error);

} // namespace vinput::cli
