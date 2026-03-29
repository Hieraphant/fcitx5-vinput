#include "mainwindow.h"

#include <QDesktopServices>
#include <QFile>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

#include "pages/control/control_page.h"
#include "pages/hotwords/hotword_page.h"
#include "pages/llm/llm_page.h"
#include "pages/resources/resource_page.h"
#include "common/utils/path_utils.h"
#include "gui/utils/config_manager.h"
#include "gui/utils/i18n_cache.h"
#include "cli/runtime/systemd_client.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle(tr("Vinput Configuration"));

  // Start background fetch of i18n map.
  vinput::gui::I18nCache::Get().Initialize(vinput::gui::ConfigManager::Get().Load());

  auto *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);
  auto *mainLayout = new QVBoxLayout(centralWidget);

  tabWidget_ = new QTabWidget(this);
  mainLayout->addWidget(tabWidget_);

  controlPage_ = new vinput::gui::ControlPage(this);
  resourcePage_ = new vinput::gui::ResourcePage(this);
  llmPage_ = new vinput::gui::LlmPage(this);
  hotwordPage_ = new vinput::gui::HotwordPage(this);

  tabWidget_->addTab(controlPage_, tr("Control"));
  tabWidget_->addTab(resourcePage_, tr("Resources"));
  tabWidget_->addTab(llmPage_, tr("LLM"));
  tabWidget_->addTab(hotwordPage_, tr("Hotwords"));

  // Cross-page refresh: any config change reloads affected pages.
  connect(controlPage_, &vinput::gui::ControlPage::configChanged, this,
          &MainWindow::reloadAll);
  connect(resourcePage_, &vinput::gui::ResourcePage::configChanged, this,
          &MainWindow::reloadAll);
  connect(llmPage_, &vinput::gui::LlmPage::configChanged, this,
          &MainWindow::reloadAll);

  // Bottom bar
  auto *bottomLayout = new QHBoxLayout();
  auto *btnOpenConfig = new QPushButton(tr("Open Config"), this);
  connect(btnOpenConfig, &QPushButton::clicked, this,
          &MainWindow::onOpenConfigClicked);

  auto *btnSave = new QPushButton(tr("Save Settings"), this);
  connect(btnSave, &QPushButton::clicked, this, &MainWindow::onSaveClicked);

  bottomLayout->addWidget(btnOpenConfig);
  bottomLayout->addStretch();
  bottomLayout->addWidget(btnSave);
  mainLayout->addLayout(bottomLayout);

  // Initial load
  controlPage_->reload();
}

MainWindow::~MainWindow() = default;

void MainWindow::reloadAll() {
  controlPage_->reload();
  llmPage_->reload();
}

void MainWindow::onSaveClicked() {
  CoreConfig config = vinput::gui::ConfigManager::Get().Load();

  // Save device
  QString device = controlPage_->currentDevice();
  if (!device.isEmpty() && device != "default") {
    config.global.captureDevice = device.toStdString();
  } else {
    config.global.captureDevice.clear();
  }

  // Save hotwords
  QString hotwordsFile = hotwordPage_->hotwordsFilePath();
  for (auto& prov : config.asr.providers) {
    if (auto* local = std::get_if<LocalAsrProvider>(&prov)) {
      if (!hotwordsFile.isEmpty()) {
        local->hotwordsFile = hotwordsFile.toStdString();
      } else {
        local->hotwordsFile.clear();
      }
    }
  }
  if (!hotwordsFile.isEmpty()) {
    // Write hotwords content to file
    QFile f(hotwordsFile);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream(&f) << hotwordPage_->hotwordsContent();
    }
  }

  if (!vinput::gui::ConfigManager::Get().Save(config)) {
    QMessageBox::critical(this, tr("Error"), tr("Failed to save config."));
    return;
  }

  // Restart daemon to apply
  vinput::cli::SystemctlRestart();
  QMessageBox::information(this, tr("Success"),
                           tr("Settings saved successfully!"));
  close();
}

void MainWindow::onOpenConfigClicked() {
  const auto configPath = vinput::path::CoreConfigPath();
  QDesktopServices::openUrl(
      QUrl::fromLocalFile(QString::fromStdString(configPath.string())));
}
