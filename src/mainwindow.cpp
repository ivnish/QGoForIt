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
#include <QEvent>
#include <QHash>
#include <QHBoxLayout>
#include <QImage>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QTimer>
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

  toolbarArea = new QWidget;
  toolbarArea->setFixedHeight(50);
  toolbarArea->setObjectName("toolbarContainer");

  auto *tbLayout = new QHBoxLayout(toolbarArea);
  tbLayout->setContentsMargins(0, 8, 0, 7);
  tbLayout->setSpacing(0);
  tbLayout->setAlignment(Qt::AlignCenter);

  todoButton = new QToolButton(toolbarArea);
  todoButton->setText(tr("To-Do"));
  todoButton->setCheckable(true);
  todoButton->setChecked(true);

  timerButton = new QToolButton(toolbarArea);
  timerButton->setText(tr("Timer"));
  timerButton->setCheckable(true);

  doneButton = new QToolButton(toolbarArea);
  doneButton->setText(tr("Done"));
  doneButton->setCheckable(true);

  // Styles must be applied before the sizeHint measurement below:
  // the stylesheet padding is part of the button width.
  applyToolbarTheme(Theme::resolve(QApplication::palette()));

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

  // The toolbar theme was applied before the sizeHint measurement above;
  // here only the pages need their styles.
  applyPagesTheme(Theme::resolve(QApplication::palette()));
}

void MainWindow::applyTheme() {
  // QApplication's palette is always current; widget palettes may lag
  // while the palette change is still being propagated.
  const Theme::Palette t = Theme::resolve(QApplication::palette());

  applyToolbarTheme(t);
  applyPagesTheme(t);
}

void MainWindow::applyPagesTheme(const Theme::Palette &t) {
  for (int i = 0; i < stackedWidget->count(); ++i) {
    if (auto *page = stackedWidget->widget(i)) {
      if (auto *todo = qobject_cast<TodoWidget *>(page)) {
        todo->retheme(t);
      } else if (auto *timer = qobject_cast<TimerWidget *>(page)) {
        timer->retheme(t);
      } else if (auto *done = qobject_cast<DoneWidget *>(page)) {
        done->retheme(t);
      }
    }
  }
}

namespace {
std::optional<QColor> dominantMenuBarColor(QMenuBar *mb) {
  // The toolbar strip must match the natively styled menu bar: platform
  // themes may paint menus with colors not exposed through classic
  // QPalette roles (e.g. Breeze on Plasma 6). The most frequent pixel of
  // the bar render is its background; item texts are a tiny minority.
  if (!mb || mb->width() <= 0 || mb->height() <= 0) return std::nullopt;
  const QImage img = mb->grab().toImage();
  QHash<QRgb, int> counts;
  for (int y = 0; y < img.height(); y += 2) {
    for (int x = 0; x < img.width(); x += 4) {
      ++counts[img.pixel(x, y)];
    }
  }
  QColor best;
  int bestCount = 0;
  QHashIterator<QRgb, int> it(counts);
  while (it.hasNext()) {
    it.next();
    if (it.value() > bestCount) {
      bestCount = it.value();
      best = QColor::fromRgba(it.key());
    }
  }
  return best;
}
}  // namespace

void MainWindow::applyToolbarTheme(const Theme::Palette &t) {
  // QMenuBar is left to the platform theme so menus behave like in other
  // system applications (native highlight and menu font).
  //
  // Native menu bar background samples are cached per palette. The
  // constructor's sample is the authoritative one: by then the menubar
  // is fully laid out and painted natively. Later samples are taken
  // only after a palette change (scheduled in scheduleThemeRefresh())
  // and are accepted after two consecutive agreeing values. A sample
  // equal to the Window role is the signature of an unnoticed transient
  // render; when the theme really paints the menu bar in the Window
  // color, the fallback below yields the identical visual result.
  const auto menuBg = dominantMenuBarColor(menuBar());
  const QColor key = QApplication::palette().color(QPalette::Window);
  if (key != stripPaletteKey) {
    // Palette changed: drop the previous theme's cached colors.
    appliedStrip = QColor();
    lastSample = QColor();
  }
  const QColor stripSample = menuBg ? *menuBg : QColor();
  if (stripSample.isValid() && stripSample == lastSample &&
      stripSample != t.window) {
    // Two consecutive samples agree: the menubar render is stable.
    appliedStrip = stripSample;
    stripPaletteKey = key;
  }
  lastSample = stripSample;
  const QColor strip = appliedStrip.isValid() ? appliedStrip : t.window;

  toolbarArea->setStyleSheet(Styles::toolbar(t, strip));
  todoButton->setStyleSheet(Styles::buttonTodo(t));
  timerButton->setStyleSheet(Styles::buttonTimer(t));
  doneButton->setStyleSheet(Styles::buttonDone(t));
}

bool MainWindow::event(QEvent *event) {
  if (event->type() == QEvent::PaletteChange) {
    scheduleThemeRefresh();
  }
  return QMainWindow::event(event);
}

void MainWindow::scheduleThemeRefresh() {
  // Widget palettes are still stale during delivery, and native
  // decorations (Breeze) repaint across several frames after a theme
  // switch. The theme is re-applied a few times over a short window;
  // each pass re-samples the menu bar, and a sample is cached only when
  // two consecutive samples agree — so a catch of a mid-repaint frame
  // cannot pin a wrong color until the next palette change.
  for (const int delay : {0, 150, 400}) {
    QTimer::singleShot(delay, this, [this]() { applyTheme(); });
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
