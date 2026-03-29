#include "cli/config/llm_actions.h"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "cli/utils/cli_helpers.h"
#include "cli/utils/resource_utils.h"
#include "cli/runtime/dbus_client.h"
#include "common/config/core_config.h"
#include "common/i18n.h"
#include "common/llm/adapter_manager.h"
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

int RunLlmConfigListAdapters(bool available, Formatter &fmt,
                             const CliContext &ctx) {
  CoreConfig config = LoadCoreConfig();
  const auto installed_display_map =
      vinput::cli::FetchScriptDisplayMap(config, vinput::script::Kind::kLlmAdapter);

  if (!available) {
    if (ctx.json_output) {
      nlohmann::json arr = nlohmann::json::array();
      for (const auto &adapter : config.llm.adapters) {
        const auto it = installed_display_map.find(adapter.id);
        arr.push_back({
            {"id", vinput::cli::HumanizeResourceId(installed_display_map,
                                                   adapter.id)},
            {"machine_id", adapter.id},
            {"title", it == installed_display_map.end() ? "" : it->second.title},
            {"readme_url",
             it == installed_display_map.end() ? "" : it->second.readme_url},
            {"command", adapter.command},
            {"args", adapter.args},
            {"env", adapter.env},
            {"running", vinput::adapter::IsRunning(adapter.id)},
        });
      }
      fmt.PrintJson(arr);
      return 0;
    }

    std::vector<std::string> headers = {_("ID"), _("TITLE"),
                                        _("README")};
    std::vector<std::vector<std::string>> rows;
    for (const auto &adapter : config.llm.adapters) {
      rows.push_back({vinput::cli::HumanizeResourceId(installed_display_map,
                                                     adapter.id),
                      installed_display_map.count(adapter.id) == 0
                          ? ""
                          : installed_display_map.at(adapter.id).title,
                      installed_display_map.count(adapter.id) == 0
                          ? ""
                          : vinput::cli::FormatTerminalLink(
                                ctx, _("Open README"),
                                installed_display_map.at(adapter.id).readme_url)});
    }
    fmt.PrintTable(headers, rows);
    return 0;
  }

  const auto registryUrls = ResolveLlmAdapterRegistryUrls(config);
  if (registryUrls.empty()) {
    fmt.PrintError(
        _("No LLM adapter registry base URLs configured. Edit config.json and set registry.base_urls."));
    return 1;
  }

  std::string error;
  const auto entries = vinput::script::FetchRegistry(
      config, vinput::script::Kind::kLlmAdapter, registryUrls, &error);
  if (!error.empty()) {
    fmt.PrintError(error);
    return 1;
  }

  const auto locale = vinput::registry::DetectPreferredLocale();
  const auto i18nMap = vinput::registry::FetchMergedI18nMap(config, locale);
  const auto display_map = vinput::cli::BuildScriptDisplayMap(entries, i18nMap);

  auto isInstalled = [&config](const std::string &id) {
    return ResolveLlmAdapter(config, id) != nullptr;
  };

  if (ctx.json_output) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &entry : entries) {
      nlohmann::json envs = nlohmann::json::array();
      for (const auto &env : entry.envs) {
        envs.push_back({{"name", env.name}, {"required", env.required}});
      }
      arr.push_back({
          {"id", vinput::cli::HumanizeResourceId(entry.id, entry.short_id)},
          {"machine_id", entry.id},
          {"title", display_map.at(entry.id).title},
          {"description", display_map.at(entry.id).description},
          {"readme_url", entry.readme_url},
          {"envs", envs},
          {"status", isInstalled(entry.id) ? "installed" : "available"},
      });
    }
    fmt.PrintJson(arr);
    return 0;
  }

  std::vector<std::string> headers = {_("ID"), _("TITLE"),
                                      _("STATUS"), _("README")};
  std::vector<std::vector<std::string>> rows;
  for (const auto &entry : entries) {
    rows.push_back({vinput::cli::HumanizeResourceId(entry.id, entry.short_id),
                    display_map.at(entry.id).title,
                    isInstalled(entry.id) ? _("installed") : _("available"),
                    vinput::cli::FormatTerminalLink(ctx, _("Open README"),
                                                    entry.readme_url)});
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

int RunLlmConfigInstallAdapter(const std::string &selector, Formatter &fmt,
                               const CliContext &ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();
  NormalizeCoreConfig(&config);

  const auto registryUrls = ResolveLlmAdapterRegistryUrls(config);
  if (registryUrls.empty()) {
    fmt.PrintError(
        _("No LLM adapter registry base URLs configured. Edit config.json and set registry.base_urls."));
    return 1;
  }

  std::string error;
  const auto entries = vinput::script::FetchRegistry(
      config, vinput::script::Kind::kLlmAdapter, registryUrls, &error);
  if (!error.empty()) {
    fmt.PrintError(error);
    return 1;
  }

  const std::string id = vinput::cli::ResolveScriptSelectorByShortId(
      selector, entries, "LLM adapter", &error);
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
        vinput::str::FmtStr(_("Adapter '%s' not found in registry."), id));
    return 1;
  }

  std::filesystem::path scriptPath;
  if (!vinput::script::DownloadScript(*it, vinput::script::Kind::kLlmAdapter,
                                      &scriptPath, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  if (!vinput::script::MaterializeLlmAdapter(&config, *it, scriptPath,
                                             &error)) {
    fmt.PrintError(error);
    return 1;
  }
  NormalizeCoreConfig(&config);
  if (!SaveConfigOrFail(config, fmt)) {
    return 1;
  }

  fmt.PrintSuccess(vinput::str::FmtStr(_("Adapter '%s' added."), selector));
  return 0;
}

int RunLlmConfigStartAdapter(const std::string &id, Formatter &fmt,
                             const CliContext &ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();
  std::string error;
  const std::string resolved_id =
      vinput::cli::ResolveInstalledLlmAdapterSelector(config, id, &error);
  if (resolved_id.empty()) {
    fmt.PrintError(error);
    return 1;
  }
  vinput::cli::DbusClient dbus;
  if (!dbus.StartAdapter(resolved_id, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  fmt.PrintSuccess(vinput::str::FmtStr(_("Adapter '%s' started."), id));
  return 0;
}

int RunLlmConfigStopAdapter(const std::string &id, Formatter &fmt,
                            const CliContext &ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();
  std::string error;
  const std::string resolved_id =
      vinput::cli::ResolveInstalledLlmAdapterSelector(config, id, &error);
  if (resolved_id.empty()) {
    fmt.PrintError(error);
    return 1;
  }
  vinput::cli::DbusClient dbus;
  if (!dbus.StopAdapter(resolved_id, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  fmt.PrintSuccess(vinput::str::FmtStr(_("Adapter '%s' stopped."), id));
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

int RunLlmConfigEdit(const std::string &id, const std::string &baseUrl,
                     const std::string &apiKey, bool hasBaseUrl, bool hasApiKey,
                     Formatter &fmt, const CliContext &ctx) {
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

  if (hasBaseUrl) {
    it->base_url = baseUrl;
  }
  if (hasApiKey) {
    it->api_key = apiKey;
  }

  if (!SaveConfigOrFail(config, fmt)) {
    return 1;
  }
  fmt.PrintSuccess(vinput::str::FmtStr(_("LLM provider '%s' updated."), id));
  return 0;
}
