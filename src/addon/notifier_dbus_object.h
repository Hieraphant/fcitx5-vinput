#pragma once

#include "common/dbus_interface.h"

#include <fcitx-utils/dbus/objectvtable.h>

#include <functional>
#include <string>

class VinputNotifierDBusObject
    : public fcitx::dbus::ObjectVTable<VinputNotifierDBusObject> {
public:
  explicit VinputNotifierDBusObject(
      std::function<void(const std::string &)> notify_callback)
      : notify_callback_(std::move(notify_callback)) {}

  void NotifyError(const std::string &message) {
    if (!message.empty() && notify_callback_) {
      notify_callback_(message);
    }
  }

private:
  FCITX_OBJECT_VTABLE_METHOD(NotifyError, vinput::dbus::kMethodNotifyError, "s",
                             "");

  std::function<void(const std::string &)> notify_callback_;
};
