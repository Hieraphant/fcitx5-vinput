#pragma once

#include <QPushButton>
#include <QProcess>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidget>

class QJsonDocument;

namespace vinput::gui {

class ResourcePage : public QWidget {
  Q_OBJECT

public:
  explicit ResourcePage(QWidget *parent = nullptr);

  void reload();

signals:
  void configChanged();

private slots:
  void refreshAll();
  void onUseModelClicked();
  void onRemoveModelClicked();
  void onDownloadModelClicked();
  void onAddProviderClicked();
  void onAddAdapterClicked();
  void onProcessReadyReadStandardOutput();
  void onProcessReadyReadStandardError();
  void onProcessFinished(int exitCode, int exitStatus);

private:
  void populateLocalModels(const QJsonDocument &doc);
  void populateRemoteModels(const QJsonDocument &doc);
  void populateRemoteProviders(const QJsonDocument &doc);
  void populateRemoteAdapters(const QJsonDocument &doc);
  void killCliProcess();
  void ensureCliProcess();

  QTableWidget *tableInstalledModels_;
  QTableWidget *tableAvailableModels_;
  QTableWidget *tableAvailableProviders_;
  QTableWidget *tableAvailableAdapters_;
  QTextEdit *textLog_;
  QPushButton *btnUseModel_;
  QPushButton *btnRemoveModel_;
  QPushButton *btnDownloadModel_;
  QPushButton *btnAddProvider_;
  QPushButton *btnAddAdapter_;
  QPushButton *btnRefreshResources_;
  QProcess *cliProcess_ = nullptr;
};

}  // namespace vinput::gui
