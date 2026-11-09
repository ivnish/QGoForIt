#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "config.h"
#include "utils.h"

#include <QMainWindow>

class QCloseEvent;
class QEvent;
class QSettings;
class QStackedWidget;
class QButtonGroup;
class QToolButton;
class SettingsDialog;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr);

 private:
  void showAbout();
  void openSettings();
  void closeEvent(QCloseEvent *event) override;
  bool event(QEvent *event) override;
  void scheduleThemeRefresh();
  void applyTheme();
  void applyPagesTheme(const Theme::Palette &t);
  void applyToolbarTheme(const Theme::Palette &t);

  QSettings *settings;
  SettingsDialog *settingsDialog;
  QColor appliedStrip;
  QColor stripPaletteKey;
  QColor lastSample;
  QToolButton *todoButton;
  QToolButton *timerButton;
  QToolButton *doneButton;
  QWidget *toolbarArea;
  QStackedWidget *stackedWidget;
  QButtonGroup *buttonGroup;
};
#endif  // MAINWINDOW_H
