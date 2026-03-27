#pragma once

#include "cli/cli_context.h"
#include "cli/formatter.h"

int RunRegistryStatus(Formatter &fmt, const CliContext &ctx);
int RunRegistrySync(Formatter &fmt, const CliContext &ctx);
