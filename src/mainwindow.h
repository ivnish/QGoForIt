#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "config.h"
#include <QMainWindow>

class QCloseEvent;
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

  QSettings *settings;
  SettingsDialog *settingsDialog;
  QToolButton *todoButton;
  QToolButton *timerButton;
  QToolButton *doneButton;
  QStackedWidget *stackedWidget;
  QButtonGroup *buttonGroup;
};
#endif // MAINWINDOW_H
