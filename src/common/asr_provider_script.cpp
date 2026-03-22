#include "common/asr_provider_script.h"

#include "config.h"
#include "common/path_utils.h"

namespace vinput::asr::script {

namespace fs = std::filesystem;

namespace {

fs::path BuiltinAsrProviderDir() {
  const fs::path installed = VINPUT_ASR_PROVIDER_INSTALL_DIR;
  std::error_code ec;
  if (fs::exists(installed, ec) && !ec) {
    return installed;
  }
  return fs::path(VINPUT_ASR_PROVIDER_SOURCE_DIR);
}

bool IsExecutableFile(const fs::path &path) {
  std::error_code ec;
  const auto status = fs::status(path, ec);
  if (ec || status.type() != fs::file_type::regular) {
    return false;
  }
  const auto perms = status.permissions();
  using perms_t = fs::perms;
  return (perms & perms_t::owner_exec) != perms_t::none ||
         (perms & perms_t::group_exec) != perms_t::none ||
         (perms & perms_t::others_exec) != perms_t::none;
}

bool MatchesCommand(const fs::path &path, std::string_view command) {
  return path.filename() == command || path.stem() == command;
}

std::optional<fs::path> ResolvePathInDir(const fs::path &dir,
                                         std::string_view command,
                                         std::string *error) {
  std::error_code ec;
  if (!fs::exists(dir, ec) || ec || !fs::is_directory(dir, ec)) {
    return std::nullopt;
  }

  const fs::path exact_path = dir / command;
  if (MatchesCommand(exact_path, command) && IsExecutableFile(exact_path)) {
    if (error) {
      error->clear();
    }
    return exact_path;
  }

  for (const auto &entry : fs::directory_iterator(dir, ec)) {
    if (ec) {
      if (error) {
        *error = "failed to scan asr provider directory: " + dir.string();
      }
      return std::nullopt;
    }
    if (!entry.is_regular_file()) {
      continue;
    }
    if (!IsExecutableFile(entry.path()) || !MatchesCommand(entry.path(), command)) {
      continue;
    }
    if (error) {
      error->clear();
    }
    return entry.path();
  }

  return std::nullopt;
}

}  // namespace

std::optional<fs::path> ResolvePath(std::string_view command, std::string *error) {
  if (command.empty()) {
    if (error) {
      *error = "asr provider command is empty";
    }
    return std::nullopt;
  }

  if (auto path =
          ResolvePathInDir(vinput::path::UserAsrProviderDir(), command, error);
      path.has_value()) {
    return path;
  }

  if (error && !error->empty()) {
    return std::nullopt;
  }

  if (auto path = ResolvePathInDir(BuiltinAsrProviderDir(), command, error);
      path.has_value()) {
    return path;
  }

  if (error && !error->empty()) {
    return std::nullopt;
  }

  if (error) {
    *error = "asr provider script not found: " + std::string(command);
  }
  return std::nullopt;
}

}  // namespace vinput::asr::script
