#include "constants.h"
#include "iconprovider.h"
#include "mainwindow.h"
#include "settingsdialog.h"
#include "timerwidget.h"
#include "todowidget.h"
#include "utils.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QToolButton>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("QGoForIt");
  setWindowIcon(IconProvider::icon("QGoForIt.svg"));
  resize(1200, 800);

  auto *fileMenu = menuBar()->addMenu(tr("&File"));
  auto *menuQuitAction = fileMenu->addAction(tr("&Quit"));
  menuQuitAction->setShortcut(QKeySequence::Quit);
  connect(menuQuitAction, &QAction::triggered, qApp, &QApplication::quit);

  auto *editMenu = menuBar()->addMenu(tr("&Edit"));
  editMenu->addAction(tr("&Preferences"), this, &MainWindow::openSettings)
      ->setShortcut(QKeySequence::Preferences);

  auto *helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(tr("&About"), this, &MainWindow::showAbout);

  auto *toolbarArea = new QWidget;
  toolbarArea->setFixedHeight(50);
  toolbarArea->setAutoFillBackground(true);
  QPalette tbPalette = toolbarArea->palette();
  tbPalette.setColor(QPalette::Window, QColor(230, 230, 230));
  toolbarArea->setPalette(tbPalette);

  auto *tbLayout = new QHBoxLayout(toolbarArea);
  tbLayout->setContentsMargins(0, 8, 0, 7);
  tbLayout->setSpacing(0);
  tbLayout->setAlignment(Qt::AlignCenter);

  todoButton = new QToolButton(toolbarArea);
  todoButton->setText(tr("To-Do"));
  todoButton->setCheckable(true);
  todoButton->setChecked(true);
  todoButton->setStyleSheet(Styles::BUTTON_TODO);

  timerButton = new QToolButton(toolbarArea);
  timerButton->setText(tr("Timer"));
  timerButton->setCheckable(true);
  timerButton->setStyleSheet(Styles::BUTTON_TIMER);

  doneButton = new QToolButton(toolbarArea);
  doneButton->setText(tr("Done"));
  doneButton->setCheckable(true);
  doneButton->setStyleSheet(Styles::BUTTON_DONE);

  const int buttonWidth =
      qMax(todoButton->sizeHint().width(),
           qMax(timerButton->sizeHint().width(),
                doneButton->sizeHint().width()));
  todoButton->setFixedSize(buttonWidth, 35);
  timerButton->setFixedSize(buttonWidth, 35);
  doneButton->setFixedSize(buttonWidth, 35);

  tbLayout->addWidget(todoButton);
  tbLayout->addWidget(timerButton);
  tbLayout->addWidget(doneButton);

  toolbarArea->setObjectName("toolbarContainer");
  toolbarArea->setStyleSheet(Styles::TOOLBAR);

  settings = Paths::settings();

  settingsDialog = new SettingsDialog(settings, this);

  auto *todoWidget = new TodoWidget(Paths::todoFile(), Paths::doneFile(), this);
  auto *timerWidget = new TimerWidget(settings, this);
  auto *doneWidget = new DoneWidget(Paths::doneFile(), Paths::todoFile(), this);

  stackedWidget = new QStackedWidget(this);
  stackedWidget->addWidget(todoWidget);    // index 0
  stackedWidget->addWidget(timerWidget);   // index 1
  stackedWidget->addWidget(doneWidget);    // index 2

  connect(todoWidget, &TodoWidget::doneListChanged, doneWidget, &DoneWidget::reload);
  connect(doneWidget, &DoneWidget::todoListChanged, todoWidget, &TodoWidget::reload);
  connect(todoWidget, &TodoWidget::activeTaskChanged, timerWidget,
          &TimerWidget::setActiveTask);

  buttonGroup = new QButtonGroup(this);
  buttonGroup->setExclusive(true);
  buttonGroup->addButton(todoButton, 0);
  buttonGroup->addButton(timerButton, 1);
  buttonGroup->addButton(doneButton, 2);
  connect(buttonGroup, &QButtonGroup::idClicked, stackedWidget,
          &QStackedWidget::setCurrentIndex);

  auto *centralWidget = new QWidget(this);
  auto *centralLayout = new QVBoxLayout(centralWidget);
  centralLayout->setContentsMargins(0, 0, 0, 0);
  centralLayout->setSpacing(0);
  centralLayout->addWidget(toolbarArea);
  centralLayout->addWidget(stackedWidget);
  setCentralWidget(centralWidget);

  if (settings->contains("window/geometry")) {
    restoreGeometry(settings->value("window/geometry").toByteArray());
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  settings->setValue("window/geometry", saveGeometry());
  QMainWindow::closeEvent(event);
}

void MainWindow::openSettings() {
  settingsDialog->loadSettings();
  if (settingsDialog->exec() == QDialog::Accepted) {
    auto *todoWidget = qobject_cast<TodoWidget *>(stackedWidget->widget(0));
    if (todoWidget) {
      todoWidget->setWatcherEnabled(settingsDialog->watcherEnabled());
    }

    auto *timerWidget = qobject_cast<TimerWidget *>(stackedWidget->widget(1));
    if (timerWidget && settingsDialog->timerSettingsChanged()) {
      timerWidget->reloadSettings();
    }

    auto *doneWidget = qobject_cast<DoneWidget *>(stackedWidget->widget(2));
    if (doneWidget) {
      doneWidget->setAutoCleanup(settingsDialog->autoCleanupEnabled(),
                                  settingsDialog->cleanupDays());
    }
  }
}

void MainWindow::showAbout() {
  const QString title = QStringLiteral("QGoForIt ") + APP_VERSION;
  const QString desc = tr("A stylish to-do list with built-in productivity timer.");
  const QString built = tr("Built with Qt") + " " + QString(qVersion());
  const QString copyright = "Copyright 2026 Andrei 'ivnish' Ivnitskii";

  const QString text = QString("<p><b>%1</b></p>"
                               "<p>%2</p>"
                               "<p>%3</p>"
                               "<p>%4</p>")
                           .arg(title, desc, built, copyright);

  QMessageBox::about(this, tr("About"), text);
}
