#include "pages/llm/llm_page.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "dialogs/adapter_dialog.h"
#include "utils/cli_runner.h"
#include "utils/gui_helpers.h"

namespace vinput::gui {

LlmPage::LlmPage(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(tr("<b>Providers</b>")));
  auto *listLayout = new QHBoxLayout();
  listProviders_ = new QListWidget();
  listLayout->addWidget(listProviders_);

  auto *btnLayout = new QVBoxLayout();
  btnLlmAdd_ = new QPushButton(tr("Add"));
  btnLlmEdit_ = new QPushButton(tr("Edit"));
  btnLlmRemove_ = new QPushButton(tr("Remove"));
  btnLayout->addWidget(btnLlmAdd_);
  btnLayout->addWidget(btnLlmEdit_);
  btnLayout->addWidget(btnLlmRemove_);
  btnLayout->addStretch();
  listLayout->addLayout(btnLayout);
  layout->addLayout(listLayout);

  auto *hint = new QLabel(tr(
      "LLM adapters are local OpenAI-compatible bridge processes. Install them "
      "from the Resources page, then manage runtime here."));
  hint->setWordWrap(true);
  layout->addWidget(hint);

  auto *adapterLayout = new QHBoxLayout();
  listAdapters_ = new QListWidget();
  adapterLayout->addWidget(listAdapters_);

  auto *adapterBtnLayout = new QVBoxLayout();
  btnAdapterEdit_ = new QPushButton(tr("Edit"));
  btnAdapterStart_ = new QPushButton(tr("Start"));
  btnAdapterStop_ = new QPushButton(tr("Stop"));
  btnAdapterRefresh_ = new QPushButton(tr("Refresh"));
  adapterBtnLayout->addWidget(btnAdapterEdit_);
  adapterBtnLayout->addWidget(btnAdapterStart_);
  adapterBtnLayout->addWidget(btnAdapterStop_);
  adapterBtnLayout->addWidget(btnAdapterRefresh_);
  adapterBtnLayout->addStretch();
  adapterLayout->addLayout(adapterBtnLayout);

  layout->addWidget(new QLabel(tr("<b>Installed Adapters</b>")));
  layout->addLayout(adapterLayout);

  connect(btnLlmAdd_, &QPushButton::clicked, this, &LlmPage::onLlmAdd);
  connect(btnLlmEdit_, &QPushButton::clicked, this, &LlmPage::onLlmEdit);
  connect(btnLlmRemove_, &QPushButton::clicked, this, &LlmPage::onLlmRemove);
  connect(btnAdapterEdit_, &QPushButton::clicked, this,
          &LlmPage::onAdapterEdit);
  connect(btnAdapterStart_, &QPushButton::clicked, this,
          &LlmPage::onAdapterStart);
  connect(btnAdapterStop_, &QPushButton::clicked, this,
          &LlmPage::onAdapterStop);
  connect(btnAdapterRefresh_, &QPushButton::clicked, this,
          &LlmPage::refreshAdapterList);

  QTimer::singleShot(0, this, &LlmPage::refreshAdapterList);
  QTimer::singleShot(0, this, &LlmPage::refreshLlmList);
}

void LlmPage::reload() {
  refreshLlmList();
  refreshAdapterList();
}

void LlmPage::refreshLlmList() {
  listProviders_->clear();

  QJsonDocument doc;
  if (!RunVinputJson({"llm", "list"}, &doc) || !doc.isArray()) {
    return;
  }

  for (const auto &v : doc.array()) {
    if (!v.isObject())
      continue;
    QJsonObject obj = v.toObject();
    QString name = obj.value("id").toString();
    QString base_url = obj.value("base_url").toString();
    QString display = QString("%1 @ %2").arg(name, base_url);

    auto *item = new QListWidgetItem(display, listProviders_);
    item->setData(Qt::UserRole, name);
  }
}

void LlmPage::refreshAdapterList() {
  listAdapters_->clear();

  QJsonDocument doc;
  if (!RunVinputJson({"adapter", "list"}, &doc) || !doc.isArray()) {
    return;
  }

  for (const auto &v : doc.array()) {
    if (!v.isObject())
      continue;
    QJsonObject obj = v.toObject();
    QString id = obj.value("id").toString();
    bool running = obj.value("running").toBool(false);

    QString display = id + " · " +
                      (running ? GuiTranslate("running")
                               : GuiTranslate("stopped"));
    QString command = obj.value("command").toString();
    if (!command.isEmpty()) {
      display += " · " + command;
    }

    auto *item = new QListWidgetItem(display, listAdapters_);
    item->setData(Qt::UserRole, id);
    item->setData(Qt::UserRole + 1, running);

    // Build tooltip
    QString tooltip;
    if (!command.isEmpty()) {
      tooltip += "\n" + tr("Command: %1").arg(command);
    }
    QJsonArray argsArr = obj.value("args").toArray();
    if (!argsArr.isEmpty()) {
      QStringList argsList;
      for (const auto &a : argsArr)
        argsList << a.toString();
      tooltip += "\n" + tr("Args: %1").arg(argsList.join(" "));
    }
    QJsonObject envObj = obj.value("env").toObject();
    if (!envObj.isEmpty()) {
      QStringList envList;
      for (auto it = envObj.begin(); it != envObj.end(); ++it)
        envList << it.key() + "=" + it.value().toString();
      tooltip += "\n" + tr("Env: %1").arg(envList.join(" "));
    }
    item->setToolTip(tooltip.trimmed());
  }
}

void LlmPage::onLlmAdd() {
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Add LLM Provider"));

  auto *form = new QFormLayout();
  auto *editName = new QLineEdit();
  auto *editBaseUrl = new QLineEdit();
  auto *editApiKey = new QLineEdit();
  editApiKey->setEchoMode(QLineEdit::Password);

  form->addRow(tr("Name:"), editName);
  form->addRow(tr("Base URL:"), editBaseUrl);
  form->addRow(tr("API Key:"), editApiKey);

  auto *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  auto *dlgLayout = new QVBoxLayout(&dialog);
  dlgLayout->addLayout(form);
  dlgLayout->addWidget(buttons);

  if (dialog.exec() != QDialog::Accepted)
    return;

  const QString name_text = editName->text().trimmed();
  const QString base_url_text = editBaseUrl->text().trimmed();
  QString validation_error;
  if (!ValidateProviderInput(name_text, base_url_text, &validation_error)) {
    QMessageBox::warning(this, tr("Error"), validation_error);
    return;
  }

  QStringList args = {"llm", "add", name_text, "-u", base_url_text};
  QString api_key = editApiKey->text();
  if (!api_key.isEmpty()) {
    args << "-k" << api_key;
  }

  QString error;
  if (!RunVinputCommand(args, &error)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }
  refreshLlmList();
  emit configChanged();
}

void LlmPage::onLlmEdit() {
  auto *item = listProviders_->currentItem();
  if (!item)
    return;

  QString provider_name = item->data(Qt::UserRole).toString();

  // Load current data from CLI
  QJsonDocument doc;
  if (!RunVinputJson({"llm", "list"}, &doc) || !doc.isArray()) {
    return;
  }

  QString current_base_url;
  for (const auto &v : doc.array()) {
    QJsonObject obj = v.toObject();
    if (obj.value("id").toString() == provider_name) {
      current_base_url = obj.value("base_url").toString();
      break;
    }
  }

  QDialog dialog(this);
  dialog.setWindowTitle(tr("Edit LLM Provider"));

  auto *form = new QFormLayout();
  auto *editName = new QLineEdit(provider_name);
  editName->setReadOnly(true);
  auto *editBaseUrl = new QLineEdit(current_base_url);
  auto *editApiKey = new QLineEdit();
  editApiKey->setEchoMode(QLineEdit::Password);
  editApiKey->setPlaceholderText(tr("Leave empty to keep current key"));

  form->addRow(tr("Name:"), editName);
  form->addRow(tr("Base URL:"), editBaseUrl);
  form->addRow(tr("API Key:"), editApiKey);

  auto *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  auto *dlgLayout = new QVBoxLayout(&dialog);
  dlgLayout->addLayout(form);
  dlgLayout->addWidget(buttons);

  if (dialog.exec() != QDialog::Accepted)
    return;

  const QString base_url_text = editBaseUrl->text().trimmed();
  QString validation_error;
  if (!ValidateProviderInput(provider_name, base_url_text, &validation_error)) {
    QMessageBox::warning(this, tr("Error"), validation_error);
    return;
  }

  QStringList args = {"llm", "edit", provider_name, "-u", base_url_text};
  QString api_key = editApiKey->text();
  if (!api_key.isEmpty()) {
    args << "-k" << api_key;
  }

  QString error;
  if (!RunVinputCommand(args, &error)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }
  refreshLlmList();
  emit configChanged();
}

void LlmPage::onLlmRemove() {
  auto *item = listProviders_->currentItem();
  if (!item)
    return;

  QString provider_name = item->data(Qt::UserRole).toString();
  auto response = QMessageBox::question(
      this, tr("Confirm"),
      tr("Are you sure you want to remove LLM provider '%1'?")
          .arg(provider_name));
  if (response != QMessageBox::Yes)
    return;

  QString error;
  if (!RunVinputCommand({"llm", "remove", provider_name}, &error)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }
  refreshLlmList();
  emit configChanged();
}

void LlmPage::onAdapterEdit() {
  auto *item = listAdapters_->currentItem();
  if (!item)
    return;

  QString adapter_id = item->data(Qt::UserRole).toString();

  // Load current data from CLI
  QJsonDocument doc;
  if (!RunVinputJson({"adapter", "list"}, &doc) || !doc.isArray()) {
    return;
  }

  AdapterData current;
  bool found = false;
  for (const auto &v : doc.array()) {
    QJsonObject obj = v.toObject();
    if (obj.value("id").toString() == adapter_id) {
      current.id = adapter_id.toStdString();
      current.command = obj.value("command").toString().toStdString();
      for (const auto &a : obj.value("args").toArray())
        current.args.push_back(a.toString().toStdString());
      QJsonObject envObj = obj.value("env").toObject();
      for (auto it = envObj.begin(); it != envObj.end(); ++it)
        current.env[it.key().toStdString()] =
            it.value().toString().toStdString();
      found = true;
      break;
    }
  }
  if (!found) {
    QMessageBox::warning(
        this, tr("Error"),
        tr("Adapter '%1' not found in configuration.").arg(adapter_id));
    return;
  }

  AdapterData updated;
  if (!ShowAdapterDialog(this, current, &updated)) {
    return;
  }

  // Use config set to update the adapter fields
  // Find the index of this adapter in the config
  QJsonDocument configDoc;
  if (!RunVinputJson({"adapter", "list"}, &configDoc) ||
      !configDoc.isArray()) {
    return;
  }

  int idx = -1;
  for (int i = 0; i < configDoc.array().size(); ++i) {
    if (configDoc.array()[i].toObject().value("id").toString() ==
            adapter_id ||
        configDoc.array()[i].toObject().value("machine_id").toString() ==
            adapter_id) {
      idx = i;
      break;
    }
  }
  if (idx < 0)
    return;

  // Get the machine_id for config path
  QString machine_id =
      configDoc.array()[idx].toObject().value("machine_id").toString();
  if (machine_id.isEmpty())
    machine_id = adapter_id;

  // Build JSON for the adapter config
  QJsonObject adapterObj;
  adapterObj["id"] = QString::fromStdString(updated.id);
  adapterObj["command"] = QString::fromStdString(updated.command);
  QJsonArray argsArr;
  for (const auto &a : updated.args)
    argsArr.append(QString::fromStdString(a));
  adapterObj["args"] = argsArr;
  QJsonObject envObj;
  for (const auto &[k, v] : updated.env)
    envObj[QString::fromStdString(k)] = QString::fromStdString(v);
  adapterObj["env"] = envObj;

  QString path = QString("/llm/adapters/%1").arg(idx);
  QString error;
  if (!RunVinputCommand(
          {"config", "set", path,
           QString::fromUtf8(
               QJsonDocument(adapterObj).toJson(QJsonDocument::Compact))},
          &error)) {
    QMessageBox::critical(this, tr("Error"), error);
    return;
  }

  refreshAdapterList();
  emit configChanged();
}

void LlmPage::onAdapterStart() {
  auto *item = listAdapters_->currentItem();
  if (!item)
    return;

  const QString adapter_id = item->data(Qt::UserRole).toString();
  QString error;
  if (!RunVinputCommand({"adapter", "start", adapter_id}, &error, -1)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }

  QMessageBox::information(this, tr("LLM Adapter Started"),
                           tr("Adapter '%1' started.").arg(adapter_id));
  refreshAdapterList();
}

void LlmPage::onAdapterStop() {
  auto *item = listAdapters_->currentItem();
  if (!item)
    return;

  const QString adapter_id = item->data(Qt::UserRole).toString();
  QString error;
  if (!RunVinputCommand({"adapter", "stop", adapter_id}, &error, -1)) {
    QMessageBox::warning(this, tr("Error"), error);
    return;
  }
  refreshAdapterList();
}

}  // namespace vinput::gui
