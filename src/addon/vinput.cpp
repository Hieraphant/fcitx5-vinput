#include "vinput.h"
#include "common/core_config.h"
#include "common/dbus_interface.h"
#include "common/i18n.h"
#include "common/postprocess_scene.h"

#include <dbus_public.h>
#include <fcitx-utils/event.h>
#include <fcitx/inputcontext.h>

#include <cstdio>
#include <string>

using namespace vinput::dbus;

VinputEngine::VinputEngine(fcitx::Instance *instance) : instance_(instance) {
  vinput::i18n::Init();
  reloadConfig();

  eventHandlers_.emplace_back(instance_->watchEvent(
      fcitx::EventType::InputContextKeyEvent,
      fcitx::EventWatcherPhase::PreInputMethod,
      [this](fcitx::Event &event) { handleKeyEvent(event); }));

  eventHandlers_.emplace_back(instance_->watchEvent(
      fcitx::EventType::InputContextCreated,
      fcitx::EventWatcherPhase::PreInputMethod,
      [](fcitx::Event &event) {
        auto &icEvent = static_cast<fcitx::InputContextEvent &>(event);
        auto *ic = icEvent.inputContext();
        ic->setCapabilityFlags(ic->capabilityFlags() |
                               fcitx::CapabilityFlag::SurroundingText);
      }));

  eventHandlers_.emplace_back(instance_->watchEvent(
      fcitx::EventType::InputContextDestroyed,
      fcitx::EventWatcherPhase::PreInputMethod,
      [this](fcitx::Event &event) {
        auto &icEvent = static_cast<fcitx::InputContextEvent &>(event);
        auto *ic = icEvent.inputContext();
        if (session_ && session_->ic == ic) {
          session_.reset();
        }
        if (scene_menu_ic_ == ic) {
          hideSceneMenu();
        }
        if (result_menu_ic_ == ic) {
          hideResultMenu();
        }
      }));

  auto *dbus_addon = instance_->addonManager().addon("dbus");
  if (dbus_addon) {
    bus_ = dbus_addon->call<fcitx::IDBusModule::bus>();
    setupDBusWatcher();
  }
}

VinputEngine::~VinputEngine() = default;

void VinputEngine::reloadConfig() {
  settings_ = LoadVinputSettings();
  applySettings();
}

void VinputEngine::save() { SaveVinputSettings(settings_); }

const fcitx::Configuration *VinputEngine::getConfig() const {
  rebuildUiConfig();
  return ui_config_.get();
}

void VinputEngine::setConfig(const fcitx::RawConfig &rawConfig) {
  auto config = std::make_unique<VinputConfig>(settings_);
  config->load(rawConfig, true);
  settings_ = config->settings();
  applySettings();
  SaveVinputSettings(settings_);
}

void VinputEngine::applySettings() {
  trigger_keys_ = settings_.triggerKeys;
  command_keys_ = settings_.commandKeys;
  scene_menu_key_ = settings_.sceneMenuKeys;
  reloadSceneConfig();
}

void VinputEngine::reloadSceneConfig() {
  auto core_config = LoadCoreConfig();
  scene_config_.activeSceneId = core_config.scenes.activeScene;
  scene_config_.scenes = core_config.scenes.definitions;
  active_scene_id_ = scene_config_.activeSceneId;
}

void VinputEngine::rebuildUiConfig() const {
  ui_config_ = std::make_unique<VinputConfig>(settings_);
}

fcitx::AddonInstance *
VinputEngineFactory::create(fcitx::AddonManager *manager) {
  return new VinputEngine(manager->instance());
}

#ifdef VINPUT_FCITX5_CORE_HAVE_ADDON_FACTORY_V2
FCITX_ADDON_FACTORY_V2(vinput, VinputEngineFactory);
#else
FCITX_ADDON_FACTORY(VinputEngineFactory);
#endif
