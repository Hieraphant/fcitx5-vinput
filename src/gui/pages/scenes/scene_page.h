#pragma once

#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

namespace vinput::gui {

class ScenePage : public QWidget {
  Q_OBJECT

public:
  explicit ScenePage(QWidget *parent = nullptr);

  void reload();

signals:
  void configChanged();

private slots:
  void onSceneAdd();
  void onSceneEdit();
  void onSceneRemove();
  void onSceneSetActive();

private:
  QListWidget *listScenes_;
  QPushButton *btnAdd_;
  QPushButton *btnEdit_;
  QPushButton *btnRemove_;
  QPushButton *btnSetActive_;
};

}  // namespace vinput::gui
