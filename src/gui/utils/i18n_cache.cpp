#include "gui/utils/i18n_cache.h"
#include <QtConcurrent/QtConcurrent>

namespace vinput::gui {

I18nCache& I18nCache::Get() {
    static I18nCache instance;
    return instance;
}

void I18nCache::Initialize(const CoreConfig& config) {
    // Copy config to avoid lifetime issues
    CoreConfig config_copy = config;
    QtConcurrent::run([this, config_copy]() {
        std::string locale = vinput::registry::DetectPreferredLocale();
        std::string error;
        auto map = vinput::registry::FetchMergedI18nMap(config_copy, locale, &error);
        if (error.empty()) {
            QMutexLocker locker(&mutex_);
            map_ = std::move(map);
            emit mapUpdated();
        }
    });
}

vinput::registry::I18nMap I18nCache::GetMap() const {
    QMutexLocker locker(&mutex_);
    return map_;
}

} // namespace vinput::gui
