#pragma once

#include <QObject>
#include <QMutex>

#include "common/registry/registry_i18n.h"
#include "common/config/core_config.h"

namespace vinput::gui {

class I18nCache : public QObject {
    Q_OBJECT
public:
    static I18nCache& Get();

    void Initialize(const CoreConfig& config);
    vinput::registry::I18nMap GetMap() const;

signals:
    void mapUpdated();

private:
    I18nCache() = default;
    ~I18nCache() override = default;

    mutable QMutex mutex_;
    vinput::registry::I18nMap map_;
};

} // namespace vinput::gui
