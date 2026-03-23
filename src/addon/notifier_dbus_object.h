#pragma once

#include "common/dbus_interface.h"
#include "common/error_info.h"

#include <fcitx-utils/dbus/objectvtable.h>

#include <functional>
#include <string>

class VinputNotifierDBusObject
    : public fcitx::dbus::ObjectVTable<VinputNotifierDBusObject> {
public:
  explicit VinputNotifierDBusObject(
      std::function<void(const vinput::dbus::ErrorInfo &)> notify_callback)
      : notify_callback_(std::move(notify_callback)) {}

  void NotifyError(const std::string &code, const std::string &subject,
                   const std::string &detail, const std::string &raw_message) {
    if (notify_callback_) {
      notify_callback_(
          vinput::dbus::MakeErrorInfo(code, subject, detail, raw_message));
    }
  }

private:
  FCITX_OBJECT_VTABLE_METHOD(NotifyError, vinput::dbus::kMethodNotifyError,
                             vinput::dbus::kErrorInfoSignature, "");

  std::function<void(const vinput::dbus::ErrorInfo &)> notify_callback_;
};
