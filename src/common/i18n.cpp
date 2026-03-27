#include "common/i18n.h"

#include "config.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <locale.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace vinput::i18n {

namespace {

std::vector<std::string> LocaleCandidates() {
  std::vector<std::string> candidates;
  const auto add_candidate = [&candidates](const char *value) {
    if (!value || !*value) {
      return;
    }

    std::string locale = value;
    const auto colon = locale.find(':');
    if (colon != std::string::npos) {
      locale = locale.substr(0, colon);
    }
    const auto dot = locale.find('.');
    if (dot != std::string::npos) {
      locale = locale.substr(0, dot);
    }
    const auto at = locale.find('@');
    if (at != std::string::npos) {
      locale = locale.substr(0, at);
    }
    if (locale.empty() || locale == "C" || locale == "POSIX") {
      return;
    }
    if (std::find(candidates.begin(), candidates.end(), locale) ==
        candidates.end()) {
      candidates.push_back(locale);
    }
    const auto underscore = locale.find('_');
    if (underscore != std::string::npos) {
      const auto language = locale.substr(0, underscore);
      if (!language.empty() &&
          std::find(candidates.begin(), candidates.end(), language) ==
              candidates.end()) {
        candidates.push_back(language);
      }
    }
  };

  add_candidate(std::getenv("LANGUAGE"));
  add_candidate(std::getenv("LC_ALL"));
  add_candidate(std::getenv("LC_MESSAGES"));
  add_candidate(std::getenv("LANG"));
  add_candidate(setlocale(LC_MESSAGES, nullptr));
  return candidates;
}

const char *ResolveLocaleDir() {
  const fs::path build_locale_dir = VINPUT_BUILD_LOCALEDIR;
  std::error_code ec;
  if (fs::exists(build_locale_dir, ec) && !ec) {
    for (const auto &locale : LocaleCandidates()) {
      const auto mo_path =
          build_locale_dir / locale / "LC_MESSAGES" / "fcitx5-vinput.mo";
      if (fs::exists(mo_path, ec) && !ec) {
        return VINPUT_BUILD_LOCALEDIR;
      }
    }
  }
  return VINPUT_LOCALEDIR;
}

} // namespace

void Init() {
  setlocale(LC_ALL, "");
  bindtextdomain("fcitx5-vinput", ResolveLocaleDir());
  bind_textdomain_codeset("fcitx5-vinput", "UTF-8");
  textdomain("fcitx5-vinput");
}

} // namespace vinput::i18n
