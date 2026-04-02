#pragma once

#include <string>

#include "cli/utils/cli_context.h"
#include "cli/utils/formatter.h"

int RunLlmConfigList(Formatter &fmt, const CliContext &ctx);
int RunLlmConfigListAdapters(bool available, Formatter &fmt,
                             const CliContext &ctx);
int RunLlmConfigAdd(const std::string &id, const std::string &baseUrl,
                    const std::string &apiKey, Formatter &fmt,
                    const CliContext &ctx);
int RunLlmConfigInstallAdapter(const std::string &selector, Formatter &fmt,
                               const CliContext &ctx);
int RunLlmConfigStartAdapter(const std::string &id, Formatter &fmt,
                             const CliContext &ctx);
int RunLlmConfigStopAdapter(const std::string &id, Formatter &fmt,
                            const CliContext &ctx);
int RunLlmConfigRemove(const std::string &id, Formatter &fmt,
                       const CliContext &ctx);
int RunLlmConfigEdit(const std::string &id, const std::string &baseUrl,
                     const std::string &apiKey, bool hasBaseUrl, bool hasApiKey,
                     Formatter &fmt, const CliContext &ctx);
int RunLlmConfigTest(const std::string &id, Formatter &fmt,
                     const CliContext &ctx);
