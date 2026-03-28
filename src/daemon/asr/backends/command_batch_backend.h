#pragma once

#include "common/config/core_config.h"
#include "daemon/asr/runtime/recognition_contract.h"

#include <memory>
#include <string>

namespace vinput::daemon::asr {

std::unique_ptr<AsrBackend>
CreateCommandBatchBackend(const CommandAsrProvider &provider, std::string *error);

}  // namespace vinput::daemon::asr
