#pragma once

#include <QListWidget>
#include <QPushButton>
#include <QWidget>

namespace vinput::gui {

class LlmPage : public QWidget {
  Q_OBJECT

public:
  explicit LlmPage(QWidget *parent = nullptr);

  void reload();

signals:
  void configChanged();

private slots:
  void refreshLlmList();
  void refreshAdapterList();
  void onLlmAdd();
  void onLlmEdit();
  void onLlmRemove();
  void onAdapterEdit();
  void onAdapterStart();
  void onAdapterStop();

private:
  QListWidget *listProviders_;
  QPushButton *btnLlmAdd_;
  QPushButton *btnLlmEdit_;
  QPushButton *btnLlmRemove_;

  QListWidget *listAdapters_;
  QPushButton *btnAdapterEdit_;
  QPushButton *btnAdapterStart_;
  QPushButton *btnAdapterStop_;
  QPushButton *btnAdapterRefresh_;
};

}  // namespace vinput::gui
