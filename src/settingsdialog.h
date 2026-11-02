#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;
class QSettings;
class QSpinBox;

class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SettingsDialog(QSettings *settings, QWidget *parent = nullptr);
  bool watcherEnabled() const;
  bool autoCleanupEnabled() const;
  int cleanupDays() const;
  bool timerSettingsChanged() const;
  void loadSettings();

 private:
  QSettings *settings;
  QWidget *setupGeneralTab();
  QWidget *setupTimerTab();
  void saveSettings();

  QCheckBox *fileWatcherCheckBox;
  QCheckBox *autoCleanupCheckBox;
  QSpinBox *cleanupDaysSpinBox;
  QWidget *cleanupDaysRow;

  QSpinBox *taskDurationSpinBox;
  QSpinBox *breakDurationSpinBox;
  QCheckBox *resumeAfterBreakCheckBox;
  QCheckBox *resetOnSwitchCheckBox;
  bool m_timerSettingsChanged = false;
};

#endif  // SETTINGSDIALOG_H
