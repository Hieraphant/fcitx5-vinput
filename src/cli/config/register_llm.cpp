#include <memory>

#include <CLI/CLI.hpp>

#include "cli/config/action.h"
#include "cli/config/llm_actions.h"
#include "common/i18n.h"

namespace vinput::cli::config {

void RegisterLlmCommands(CLI::App &app, CliAction *action) {
  auto *llm = app.add_subcommand("llm", _("Manage LLM provider configuration"));
  llm->require_subcommand(1);

  auto *list = llm->add_subcommand("list", _("List configured LLM providers"));
  list->alias("ls");
  list->callback([action]() {
    *action = [](Formatter &fmt, const CliContext &ctx) {
      return RunLlmConfigList(fmt, ctx);
    };
  });

  auto *listAdaptors =
      llm->add_subcommand("list-adaptors", _("List LLM adaptors"));
  auto availableAdaptors = std::make_shared<bool>(false);
  listAdaptors->add_flag("-a,--available", *availableAdaptors,
                         _("List available remote LLM adaptors"));
  listAdaptors->callback([action, availableAdaptors]() {
    *action = [availableAdaptors](Formatter &fmt, const CliContext &ctx) {
      return RunLlmConfigListAdaptors(*availableAdaptors, fmt, ctx);
    };
  });

  auto id = std::make_shared<std::string>();
  auto baseUrl = std::make_shared<std::string>();
  auto apiKey = std::make_shared<std::string>();
  auto *add = llm->add_subcommand("add", _("Add an LLM provider"));
  add->add_option("id", *id, _("Provider id"))->required();
  add->add_option("-u,--base-url", *baseUrl, _("Base URL"))->required();
  add->add_option("-k,--api-key", *apiKey, _("API key"));
  add->callback([action, id, baseUrl, apiKey]() {
    *action = [id, baseUrl, apiKey](Formatter &fmt, const CliContext &ctx) {
      return RunLlmConfigAdd(*id, *baseUrl, *apiKey, fmt, ctx);
    };
  });

  auto adaptorSelector = std::make_shared<std::string>();
  auto *installAdaptor =
      llm->add_subcommand("install-adaptor", _("Install an LLM adaptor"));
  installAdaptor->add_option("id_or_index", *adaptorSelector,
                             _("Adaptor id or available-list index"))
      ->required();
  installAdaptor->callback([action, adaptorSelector]() {
    *action = [adaptorSelector](Formatter &fmt, const CliContext &ctx) {
      return RunLlmConfigInstallAdaptor(*adaptorSelector, fmt, ctx);
    };
  });

  auto adaptorId = std::make_shared<std::string>();
  auto *startAdaptor =
      llm->add_subcommand("start-adaptor", _("Start an LLM adaptor"));
  startAdaptor->add_option("id", *adaptorId, _("Adaptor id"))->required();
  startAdaptor->callback([action, adaptorId]() {
    *action = [adaptorId](Formatter &fmt, const CliContext &ctx) {
      return RunLlmConfigStartAdaptor(*adaptorId, fmt, ctx);
    };
  });

  auto stopAdaptorId = std::make_shared<std::string>();
  auto *stopAdaptor =
      llm->add_subcommand("stop-adaptor", _("Stop an LLM adaptor"));
  stopAdaptor->add_option("id", *stopAdaptorId, _("Adaptor id"))->required();
  stopAdaptor->callback([action, stopAdaptorId]() {
    *action = [stopAdaptorId](Formatter &fmt, const CliContext &ctx) {
      return RunLlmConfigStopAdaptor(*stopAdaptorId, fmt, ctx);
    };
  });

  auto removeId = std::make_shared<std::string>();
  auto *remove = llm->add_subcommand("remove", _("Remove an LLM provider"));
  remove->alias("rm");
  remove->add_option("id", *removeId, _("Provider id"))->required();
  remove->callback([action, removeId]() {
    *action = [removeId](Formatter &fmt, const CliContext &ctx) {
      return RunLlmConfigRemove(*removeId, fmt, ctx);
    };
  });
}

}  // namespace vinput::cli::config
