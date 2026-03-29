#pragma once

#include <QThread>
#include <QString>
#include <functional>
#include <string>

namespace vinput::gui {

class DownloadWorker : public QThread {
    Q_OBJECT
public:
    explicit DownloadWorker(QObject* parent = nullptr);
    ~DownloadWorker() override;

    // The task should return true on success, or set error and return false.
    void SetTask(std::function<bool(std::string*)> task);
    
    // For tasks that report progress
    void ReportProgress(int percent, const QString& speed);

signals:
    void progress(int percent, QString speed);
    void error(QString message);

protected:
    void run() override;

private:
    std::function<bool(std::string*)> task_;
};

} // namespace vinput::gui
