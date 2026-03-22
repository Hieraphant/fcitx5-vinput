#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace vinput::asr::script {

std::optional<std::filesystem::path> ResolvePath(std::string_view command,
                                                 std::string *error);

}  // namespace vinput::asr::script
