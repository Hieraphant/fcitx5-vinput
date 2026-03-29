#include "pages/scenes/scene_page.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include "utils/cli_runner.h"
#include "utils/gui_helpers.h"

namespace vinput::gui {

namespace {

constexpr int kDefaultTimeoutMs = 30000;
constexpr int kDefaultCandidateCount = 3;
constexpr int kMinCandidateCount = 1;
constexpr int kMaxCandidateCount = 10;

QString SceneLabelForGui(const QJsonObject &scene) {
  QString label = scene.value("label").toString();
  QString id = scene.value("id").toString();
  if (label == "raw" || (label.isEmpty() && id == "raw"))
    return GuiTranslate("Raw");
  if (label == "command" || (label.isEmpty() && id == "command"))
    return GuiTranslate("Command");
  if (!label.isEmpty())
    return label;
  return id;
}

}  // namespace

ScenePage::ScenePage(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  auto *listLayout = new QHBoxLayout();
  listScenes_ = new QListWidget();
  listLayout->addWidget(listScenes_);

  auto *btnLayout = new QVBoxLayout();
  btnAdd_ = new QPushButton(tr("Add"));
  btnEdit_ = new QPushButton(tr("Edit"));
  btnRemove_ = new QPushButton(tr("Remove"));
  btnSetActive_ = new QPushButton(tr("Set Active"));
  btnLayout->addWidget(btnAdd_);
  btnLayout->addWidget(btnEdit_);
  btnLayout->addWidget(btnRemove_);
  btnLayout->addWidget(btnSetActive_);
  btnLayout->addStretch();
  listLayout->addLayout(btnLayout);
  layout->addLayout(listLayout);

  connect(btnAdd_, &QPushButton::clicked, this, &ScenePage::onSceneAdd);
  connect(btnEdit_, &QPushButton::clicked, this, &ScenePage::onSceneEdit);
  connect(btnRemove_, &QPushButton::clicked, this, &ScenePage::onSceneRemove);
  connect(btnSetActive_, &QPushButton::clicked, this,
          &ScenePage::onSceneSetActive);

  QTimer::singleShot(0, this, &ScenePage::reload);
}

void ScenePage::reload() {
  listScenes_->clear();

  QJsonDocument doc;
  if (!RunVinputJson({"scene", "list"}, &doc) || !doc.isArray()) {
    return;
  }

  for (const auto &v : doc.array()) {
    if (!v.isObject())
      continue;
    QJsonObject obj = v.toObject();
    QString id = obj.value("id").toString();
    QString label = SceneLabelForGui(obj);
    bool active = obj.value("active").toBool(false);

    QString display = label;
    if (active)
      display += " *";

    auto *item = new QListWidgetItem(display, listScenes_);
    item->setData(Qt::UserRole, id);
  }
}

void ScenePage::onSceneAdd() {
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Add Scene"));

  auto *form = new QFormLayout();
  auto *editId = new QLineEdit();
  auto *editLabel = new QLineEdit();
  auto *editPrompt = new QTextEdit();
  editPrompt->setMaximumHeight(100);
  auto *comboProvider = new QComboBox();
  auto *comboModel = new QComboBox();
  auto *spinTimeout = new QSpinBox();
  spinTimeout->setRange(1000, 300000);
  spinTimeout->setSingleStep(1000);
  spinTimeout->setValue(kDefaultTimeoutMs);
  spinTimeout->setSuffix(" ms");
  auto *spinCandidates = new QSpinBox();
  spinCandidates->setRange(kMinCandidateCount, kMaxCandidateCount);
  spinCandidates->setValue(kDefaultCandidateCount);

  SetupProviderModelCombos(comboProvider, comboModel);

  form->addRow(tr("ID:"), editId);
  form->addRow(tr("Label:"), editLabel);
  form->addRow(tr("Prompt:"), editPrompt);
  form->addRow(tr("Provider:"), comboProvider);
  form->addRow(tr("Model:"), comboModel);
  form->addRow(tr("Candidate Count:"), spinCandidates);
  form->addRow(tr("Timeout (ms):"), spinTimeout);

  auto *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  auto *dlgLayout = new QVBoxLayout(&dialog);
  dlgLayout->addLayout(form);
  dlgLayout->addWidget(buttons);

  if (dialog.exec() != QDialog::Accepted)
    return;

  QStringList args = {"scene", "add",
                      "--id", editId->text().trimmed()};
  QString label = editLabel->text().trimmed();
  if (!label.isEmpty()) {
    args << "-l" << label;
  }
  QString prompt = editPrompt->toPlainText();
  if (!prompt.isEmpty()) {
    args << "-t" << prompt;
  }
  QString provider = comboProvider->currentText();
  if (!provider.isEmpty()) {
    args << "-p" << provider;
  }
  QString model = comboModel->currentText().trimmed();
  if (!model.isEmpty()) {
    args << "-m" << model;
  }
  args << "-c" << QString::number(spinCandidates->value());
  args << "--timeout" << QString::number(spinTimeout->value());

  QString error;
  if (!RunVinputCommand(args, &error)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }
  reload();
  emit configChanged();
}

void ScenePage::onSceneEdit() {
  auto *item = listScenes_->currentItem();
  if (!item)
    return;

  QString scene_id = item->data(Qt::UserRole).toString();

  // Load current scene data from CLI
  QJsonDocument doc;
  if (!RunVinputJson({"scene", "list"}, &doc) || !doc.isArray()) {
    return;
  }

  QJsonObject found;
  bool exists = false;
  for (const auto &v : doc.array()) {
    QJsonObject obj = v.toObject();
    if (obj.value("id").toString() == scene_id) {
      found = obj;
      exists = true;
      break;
    }
  }
  if (!exists)
    return;

  QDialog dialog(this);
  dialog.setWindowTitle(tr("Edit Scene"));

  auto *form = new QFormLayout();
  auto *editId = new QLineEdit(scene_id);
  editId->setReadOnly(true);
  auto *editLabel = new QLineEdit(SceneLabelForGui(found));
  auto *editPrompt = new QTextEdit();
  editPrompt->setPlainText(found.value("prompt").toString());
  editPrompt->setMaximumHeight(100);
  auto *comboProvider = new QComboBox();
  auto *comboModel = new QComboBox();
  auto *spinTimeout = new QSpinBox();
  spinTimeout->setRange(1000, 300000);
  spinTimeout->setSingleStep(1000);
  spinTimeout->setValue(found.value("timeout_ms").toInt(kDefaultTimeoutMs));
  spinTimeout->setSuffix(" ms");
  auto *spinCandidates = new QSpinBox();
  spinCandidates->setRange(kMinCandidateCount, kMaxCandidateCount);
  spinCandidates->setValue(
      found.value("candidate_count").toInt(kDefaultCandidateCount));

  SetupProviderModelCombos(comboProvider, comboModel,
                           found.value("provider_id").toString(),
                           found.value("model").toString());

  form->addRow(tr("ID:"), editId);
  form->addRow(tr("Label:"), editLabel);
  form->addRow(tr("Prompt:"), editPrompt);
  form->addRow(tr("Provider:"), comboProvider);
  form->addRow(tr("Model:"), comboModel);
  form->addRow(tr("Candidate Count:"), spinCandidates);
  form->addRow(tr("Timeout (ms):"), spinTimeout);

  auto *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  auto *dlgLayout = new QVBoxLayout(&dialog);
  dlgLayout->addLayout(form);
  dlgLayout->addWidget(buttons);

  if (dialog.exec() != QDialog::Accepted)
    return;

  QStringList args = {"scene", "edit", scene_id};
  args << "-l" << editLabel->text().trimmed();
  args << "-t" << editPrompt->toPlainText();
  args << "-p" << comboProvider->currentText();
  args << "-m" << comboModel->currentText().trimmed();
  args << "-c" << QString::number(spinCandidates->value());
  args << "--timeout" << QString::number(spinTimeout->value());

  QString error;
  if (!RunVinputCommand(args, &error)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }
  reload();
  emit configChanged();
}

void ScenePage::onSceneRemove() {
  auto *item = listScenes_->currentItem();
  if (!item)
    return;

  QString scene_id = item->data(Qt::UserRole).toString();
  auto response = QMessageBox::question(
      this, tr("Confirm"),
      tr("Are you sure you want to remove scene '%1'?").arg(scene_id));
  if (response != QMessageBox::Yes)
    return;

  QString error;
  if (!RunVinputCommand({"scene", "remove", scene_id}, &error)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }
  reload();
  emit configChanged();
}

void ScenePage::onSceneSetActive() {
  auto *item = listScenes_->currentItem();
  if (!item)
    return;

  QString scene_id = item->data(Qt::UserRole).toString();
  QString error;
  if (!RunVinputCommand({"scene", "use", scene_id}, &error)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }
  reload();
  emit configChanged();
}

}  // namespace vinput::gui
