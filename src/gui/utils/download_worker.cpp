#include "gui/utils/download_worker.h"

namespace vinput::gui {

DownloadWorker::DownloadWorker(QObject* parent) : QThread(parent) {}

DownloadWorker::~DownloadWorker() {
    wait();
}

void DownloadWorker::SetTask(std::function<bool(std::string*)> task) {
    task_ = std::move(task);
}

void DownloadWorker::ReportProgress(int percent, const QString& speed) {
    emit progress(percent, speed);
}

void DownloadWorker::run() {
    if (!task_) return;
    std::string error_msg;
    if (!task_(&error_msg)) {
        emit error(QString::fromStdString(error_msg));
    }
}

} // namespace vinput::gui
