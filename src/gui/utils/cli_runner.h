#pragma once

#include <QJsonDocument>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

class QProcess;

namespace vinput::gui {

// Resolve the vinput CLI executable path.
QString ResolveVinputExecutable(QString *error_out = nullptr);

// Launch vinput detached (fire-and-forget).
bool StartVinputDetached(const QStringList &args,
                         QString *error_out = nullptr);

// Run vinput --json <args> synchronously and parse JSON output.
bool RunVinputJson(const QStringList &args, QJsonDocument *out_doc,
                   QString *error_out = nullptr, int timeout_ms = 5000);

// Run vinput <args> synchronously, return success/failure.
bool RunVinputCommand(const QStringList &args, QString *error_out = nullptr,
                      int timeout_ms = 5000);

// Restart the daemon via CLI.
void RestartDaemon(const QStringList &extra_args = {});

// Check runtime environment (e.g. Flatpak permissions).
// Shows a warning dialog parented to `parent` if issues are found.
void CheckRuntimeEnvironment(QWidget *parent);

// Callback type for async JSON results: (success, doc, error).
using JsonCallback =
    std::function<void(bool ok, const QJsonDocument &doc, const QString &err)>;

// Run vinput --json <args> asynchronously. Calls `callback` on completion.
// The QProcess is parented to `parent` and auto-deleted.
// Returns the QProcess* (for optional early termination).
QProcess *RunVinputJsonAsync(const QStringList &args, QObject *parent,
                             JsonCallback callback, int timeout_ms = 15000);

}  // namespace vinput::gui
