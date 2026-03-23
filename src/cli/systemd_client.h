#pragma once

#include <string>

namespace vinput::cli {

constexpr const char* kServiceUnit = "vinput-daemon.service";

int SystemctlStart();
int SystemctlStop();
int SystemctlRestart();
int JournalctlLogs(bool follow, int lines);
std::string JournalctlLogsText(int lines);

} // namespace vinput::cli
