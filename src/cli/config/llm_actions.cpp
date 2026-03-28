#include "cli/config/llm_actions.h"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "cli/utils/cli_helpers.h"
#include "cli/runtime/dbus_client.h"
#include "common/config/core_config.h"
#include "common/i18n.h"
#include "common/registry/registry_i18n.h"
#include "common/registry/registry_scripts.h"
#include "common/utils/string_utils.h"

namespace {

std::string MaskApiKey(const std::string &key) {
  if (key.size() <= 8) {
    return std::string(key.size(), '*');
  }
  return key.substr(0, 4) + std::string(key.size() - 8, '*') +
         key.substr(key.size() - 4);
}

bool TryParseOneBasedIndex(const std::string &text, std::size_t *index) {
  if (!index || text.empty()) {
    return false;
  }
  if (!std::all_of(text.begin(), text.end(),
                   [](unsigned char ch) { return std::isdigit(ch) != 0; })) {
    return false;
  }
  try {
    const std::size_t parsed = std::stoull(text);
    if (parsed == 0) {
      return false;
    }
    *index = parsed - 1;
    return true;
  } catch (...) {
    return false;
  }
}

std::string ResolveAdaptorSelector(
    const std::string &selector,
    const std::vector<vinput::script::RegistryEntry> &entries,
    std::string *error) {
  std::size_t index = 0;
  if (TryParseOneBasedIndex(selector, &index)) {
    if (index >= entries.size()) {
      if (error) {
        *error = "available adaptor index out of range";
      }
      return {};
    }
    return entries[index].id;
  }

  for (const auto &entry : entries) {
    if (entry.id == selector) {
      return selector;
    }
  }
  if (error) {
    *error = "available adaptor not found: " + selector;
  }
  return {};
}

}  // namespace

int RunLlmConfigList(Formatter &fmt, const CliContext &ctx) {
  CoreConfig config = LoadCoreConfig();

  if (ctx.json_output) {
    nlohmann::json providers = nlohmann::json::array();
    for (const auto &provider : config.llm.providers) {
      providers.push_back({
          {"id", provider.id},
          {"base_url", provider.base_url},
          {"api_key", ""},
      });
    }
    fmt.PrintJson(providers);
    return 0;
  }

  std::vector<std::string> headers = {_("ID"), _("BASE_URL"), _("API_KEY")};
  std::vector<std::vector<std::string>> rows;
  for (const auto &provider : config.llm.providers) {
    rows.push_back({provider.id, provider.base_url, MaskApiKey(provider.api_key)});
  }
  fmt.PrintTable(headers, rows);
  return 0;
}

int RunLlmConfigListAdaptors(bool available, Formatter &fmt,
                             const CliContext &ctx) {
  CoreConfig config = LoadCoreConfig();

  if (!available) {
    if (ctx.json_output) {
      nlohmann::json arr = nlohmann::json::array();
      std::size_t index = 1;
      for (const auto &adaptor : config.llm.adaptors) {
        arr.push_back({
            {"index", index++},
            {"id", adaptor.id},
            {"command", adaptor.command},
            {"args", adaptor.args},
            {"env", adaptor.env},
        });
      }
      fmt.PrintJson(arr);
      return 0;
    }

    std::vector<std::string> headers = {_("INDEX"), _("ID"), _("COMMAND")};
    std::vector<std::vector<std::string>> rows;
    std::size_t index = 1;
    for (const auto &adaptor : config.llm.adaptors) {
      rows.push_back({std::to_string(index++), adaptor.id,
                      adaptor.command.empty() ? _("(not set)") : adaptor.command});
    }
    fmt.PrintTable(headers, rows);
    return 0;
  }

  const auto registryUrls = ResolveLlmAdaptorRegistryUrls(config);
  if (registryUrls.empty()) {
    fmt.PrintError(
        _("No LLM adaptor registry base URLs configured. Edit config.json and set registry.base_urls."));
    return 1;
  }

  std::string error;
  const auto entries = vinput::script::FetchRegistry(
      config, vinput::script::Kind::kLlmAdaptor, registryUrls, &error);
  if (!error.empty()) {
    fmt.PrintError(error);
    return 1;
  }

  const auto locale = vinput::registry::DetectPreferredLocale();
  const auto i18nMap = vinput::registry::FetchMergedI18nMap(config, locale);

  auto isInstalled = [&config](const std::string &id) {
    return ResolveLlmAdaptor(config, id) != nullptr;
  };

  if (ctx.json_output) {
    nlohmann::json arr = nlohmann::json::array();
    std::size_t index = 1;
    for (const auto &entry : entries) {
      nlohmann::json envs = nlohmann::json::array();
      for (const auto &env : entry.envs) {
        envs.push_back({{"name", env.name}, {"required", env.required}});
      }
      arr.push_back({
          {"index", index++},
          {"id", entry.id},
          {"title", vinput::registry::LookupI18n(
                        i18nMap, entry.id + ".title", entry.id)},
          {"description", vinput::registry::LookupI18n(
                              i18nMap, entry.id + ".description", "")},
          {"command", entry.command},
          {"readme_url", entry.readme_url},
          {"envs", envs},
          {"status", isInstalled(entry.id) ? "installed" : "available"},
      });
    }
    fmt.PrintJson(arr);
    return 0;
  }

  std::vector<std::string> headers = {_("INDEX"), _("ID"), _("TITLE"),
                                      _("COMMAND"), _("STATUS")};
  std::vector<std::vector<std::string>> rows;
  std::size_t index = 1;
  for (const auto &entry : entries) {
    rows.push_back({std::to_string(index++),
                    entry.id,
                    vinput::registry::LookupI18n(i18nMap, entry.id + ".title",
                                                 entry.id),
                    entry.command,
                    isInstalled(entry.id) ? _("installed") : _("available")});
  }
  fmt.PrintTable(headers, rows);
  return 0;
}

int RunLlmConfigAdd(const std::string &id, const std::string &baseUrl,
                    const std::string &apiKey, Formatter &fmt,
                    const CliContext &ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();
  if (ResolveLlmProvider(config, id) != nullptr) {
    fmt.PrintError(vinput::str::FmtStr(_("LLM provider '%s' already exists."), id));
    return 1;
  }

  LlmProvider provider;
  provider.id = id;
  provider.base_url = baseUrl;
  provider.api_key = apiKey;
  config.llm.providers.push_back(std::move(provider));

  if (!SaveConfigOrFail(config, fmt)) {
    return 1;
  }
  fmt.PrintSuccess(vinput::str::FmtStr(_("LLM provider '%s' added."), id));
  return 0;
}

int RunLlmConfigInstallAdaptor(const std::string &selector, Formatter &fmt,
                               const CliContext &ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();
  NormalizeCoreConfig(&config);

  const auto registryUrls = ResolveLlmAdaptorRegistryUrls(config);
  if (registryUrls.empty()) {
    fmt.PrintError(
        _("No LLM adaptor registry base URLs configured. Edit config.json and set registry.base_urls."));
    return 1;
  }

  std::string error;
  const auto entries = vinput::script::FetchRegistry(
      config, vinput::script::Kind::kLlmAdaptor, registryUrls, &error);
  if (!error.empty()) {
    fmt.PrintError(error);
    return 1;
  }

  const std::string id = ResolveAdaptorSelector(selector, entries, &error);
  if (id.empty()) {
    fmt.PrintError(error);
    return 1;
  }

  const auto it =
      std::find_if(entries.begin(), entries.end(),
                   [&id](const vinput::script::RegistryEntry &entry) {
                     return entry.id == id;
                   });
  if (it == entries.end()) {
    fmt.PrintError(
        vinput::str::FmtStr(_("Adaptor '%s' not found in registry."), id));
    return 1;
  }

  std::filesystem::path scriptPath;
  if (!vinput::script::DownloadScript(*it, vinput::script::Kind::kLlmAdaptor,
                                      &scriptPath, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  if (!vinput::script::MaterializeLlmAdaptor(&config, *it, scriptPath,
                                             &error)) {
    fmt.PrintError(error);
    return 1;
  }
  NormalizeCoreConfig(&config);
  if (!SaveConfigOrFail(config, fmt)) {
    return 1;
  }

  fmt.PrintSuccess(
      vinput::str::FmtStr(_("Adaptor '%s' synchronized to local config."), id));
  fmt.PrintInfo(
      vinput::str::FmtStr(_("Local script path: %s"), scriptPath.string()));
  return 0;
}

int RunLlmConfigStartAdaptor(const std::string &id, Formatter &fmt,
                             const CliContext &ctx) {
  (void)ctx;
  std::string error;
  vinput::cli::DbusClient dbus;
  if (!dbus.StartAdaptor(id, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  fmt.PrintSuccess(vinput::str::FmtStr(_("Adaptor '%s' started."), id));
  return 0;
}

int RunLlmConfigStopAdaptor(const std::string &id, Formatter &fmt,
                            const CliContext &ctx) {
  (void)ctx;
  std::string error;
  vinput::cli::DbusClient dbus;
  if (!dbus.StopAdaptor(id, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  fmt.PrintSuccess(vinput::str::FmtStr(_("Adaptor '%s' stopped."), id));
  return 0;
}

int RunLlmConfigRemove(const std::string &id, Formatter &fmt,
                       const CliContext &ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();
  auto &providers = config.llm.providers;
  const auto it = std::find_if(
      providers.begin(), providers.end(),
      [&id](const LlmProvider &provider) { return provider.id == id; });
  if (it == providers.end()) {
    fmt.PrintError(vinput::str::FmtStr(_("LLM provider '%s' not found."), id));
    return 1;
  }

  providers.erase(it);
  if (!SaveConfigOrFail(config, fmt)) {
    return 1;
  }
  fmt.PrintSuccess(vinput::str::FmtStr(_("LLM provider '%s' removed."), id));
  return 0;
}
