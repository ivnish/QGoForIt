#include "settingsdialog.h"
#include "constants.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QSettings *settings, QWidget *parent)
    : QDialog(parent), settings(settings) {
  setWindowTitle(tr("Preferences"));

  auto *tabs = new QTabWidget(this);
  tabs->addTab(setupGeneralTab(), tr("&General"));
  tabs->addTab(setupTimerTab(), tr("&Timer"));

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(tabs);

  auto *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                           this);
  connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
    saveSettings();
    accept();
  });
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);

  loadSettings();
}

QWidget *SettingsDialog::setupGeneralTab() {
  auto *tab = new QWidget(this);
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(12);

  fileWatcherCheckBox =
      new QCheckBox(tr("Watch todo.txt for external changes"), tab);
  layout->addWidget(fileWatcherCheckBox);

  autoCleanupCheckBox =
      new QCheckBox(tr("Auto-delete old completed tasks"), tab);
  layout->addWidget(autoCleanupCheckBox);

  cleanupDaysRow = new QWidget(tab);
  auto *rowLayout = new QHBoxLayout(cleanupDaysRow);
  rowLayout->setContentsMargins(16, 0, 0, 0);
  auto *daysLabel = new QLabel(tr("Delete tasks older than"), cleanupDaysRow);
  cleanupDaysSpinBox = new QSpinBox(cleanupDaysRow);
  cleanupDaysSpinBox->setRange(SettingsKeys::CLEANUP_DAYS_MIN,
                                SettingsKeys::CLEANUP_DAYS_MAX);
  cleanupDaysSpinBox->setSuffix(tr(" days"));
  rowLayout->addWidget(daysLabel);
  rowLayout->addWidget(cleanupDaysSpinBox);
  rowLayout->addStretch(1);
  layout->addWidget(cleanupDaysRow);

  connect(autoCleanupCheckBox, &QCheckBox::toggled, cleanupDaysRow,
          &QWidget::setEnabled);

  layout->addStretch(1);
  return tab;
}

QWidget *SettingsDialog::setupTimerTab() {
  auto *tab = new QWidget(this);
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(12);

  auto *durationsGrid = new QGridLayout;
  durationsGrid->setHorizontalSpacing(8);
  durationsGrid->setVerticalSpacing(12);
  int row = 0;

  auto *taskDurationLabel = new QLabel(tr("Task duration:"), tab);
  taskDurationSpinBox = new QSpinBox(tab);
  taskDurationSpinBox->setRange(SettingsKeys::TIMER_DURATION_MIN,
                                SettingsKeys::TIMER_DURATION_MAX);
  taskDurationSpinBox->setSuffix(tr(" min"));
  taskDurationSpinBox->setAccessibleName(tr("Task duration in minutes"));
  durationsGrid->addWidget(taskDurationLabel, row, 0);
  durationsGrid->addWidget(taskDurationSpinBox, row, 1);
  ++row;

  auto *breakDurationLabel = new QLabel(tr("Break duration:"), tab);
  breakDurationSpinBox = new QSpinBox(tab);
  breakDurationSpinBox->setRange(SettingsKeys::TIMER_DURATION_MIN,
                                 SettingsKeys::TIMER_DURATION_MAX);
  breakDurationSpinBox->setSuffix(tr(" min"));
  breakDurationSpinBox->setAccessibleName(tr("Break duration in minutes"));
  durationsGrid->addWidget(breakDurationLabel, row, 0);
  durationsGrid->addWidget(breakDurationSpinBox, row, 1);
  ++row;

  durationsGrid->setColumnStretch(2, 1);
  layout->addLayout(durationsGrid);

  auto *separator = new QFrame(tab);
  separator->setFrameShape(QFrame::HLine);
  separator->setFrameShadow(QFrame::Sunken);
  layout->addWidget(separator);

  resumeAfterBreakCheckBox =
      new QCheckBox(tr("Resume task after the break"), tab);
  layout->addWidget(resumeAfterBreakCheckBox);

  resetOnSwitchCheckBox =
      new QCheckBox(tr("Reset timer after switching tasks"), tab);
  layout->addWidget(resetOnSwitchCheckBox);

  layout->addStretch(1);
  return tab;
}

bool SettingsDialog::watcherEnabled() const {
  return fileWatcherCheckBox->isChecked();
}

bool SettingsDialog::autoCleanupEnabled() const {
  return autoCleanupCheckBox->isChecked();
}

int SettingsDialog::cleanupDays() const {
  return cleanupDaysSpinBox->value();
}

bool SettingsDialog::timerSettingsChanged() const {
  return m_timerSettingsChanged;
}

void SettingsDialog::loadSettings() {
  fileWatcherCheckBox->setChecked(
      settings->value(SettingsKeys::FILE_WATCHER,
                      SettingsKeys::FILE_WATCHER_DEFAULT)
          .toBool());

  autoCleanupCheckBox->setChecked(
      settings->value(SettingsKeys::AUTO_CLEANUP,
                      SettingsKeys::AUTO_CLEANUP_DEFAULT)
          .toBool());
  cleanupDaysSpinBox->setValue(
      settings->value(SettingsKeys::CLEANUP_DAYS,
                      SettingsKeys::CLEANUP_DAYS_DEFAULT)
          .toInt());
  cleanupDaysRow->setEnabled(autoCleanupCheckBox->isChecked());

  const int taskMinutes =
      settings
          ->value(SettingsKeys::TIMER_TASK_DURATION,
                  SettingsKeys::TIMER_TASK_DURATION_DEFAULT)
          .toInt();
  const int breakMinutes =
      settings
          ->value(SettingsKeys::TIMER_BREAK_DURATION,
                  SettingsKeys::TIMER_BREAK_DURATION_DEFAULT)
          .toInt();
  taskDurationSpinBox->setValue(
      qBound(SettingsKeys::TIMER_DURATION_MIN, taskMinutes,
             SettingsKeys::TIMER_DURATION_MAX));
  breakDurationSpinBox->setValue(
      qBound(SettingsKeys::TIMER_DURATION_MIN, breakMinutes,
             SettingsKeys::TIMER_DURATION_MAX));
  resumeAfterBreakCheckBox->setChecked(
      settings
          ->value(SettingsKeys::TIMER_RESUME_AFTER_BREAK,
                  SettingsKeys::TIMER_RESUME_AFTER_BREAK_DEFAULT)
          .toBool());
  resetOnSwitchCheckBox->setChecked(
      settings
          ->value(SettingsKeys::TIMER_RESET_ON_TASK_SWITCH,
                  SettingsKeys::TIMER_RESET_ON_TASK_SWITCH_DEFAULT)
          .toBool());

  m_timerSettingsChanged = false;
}

void SettingsDialog::saveSettings() {
  settings->setValue(SettingsKeys::FILE_WATCHER, fileWatcherCheckBox->isChecked());
  settings->setValue(SettingsKeys::AUTO_CLEANUP, autoCleanupCheckBox->isChecked());
  settings->setValue(SettingsKeys::CLEANUP_DAYS, cleanupDaysSpinBox->value());

  const int oldTaskDuration =
      settings
          ->value(SettingsKeys::TIMER_TASK_DURATION,
                  SettingsKeys::TIMER_TASK_DURATION_DEFAULT)
          .toInt();
  const int oldBreakDuration =
      settings
          ->value(SettingsKeys::TIMER_BREAK_DURATION,
                  SettingsKeys::TIMER_BREAK_DURATION_DEFAULT)
          .toInt();
  const bool oldResume =
      settings
          ->value(SettingsKeys::TIMER_RESUME_AFTER_BREAK,
                  SettingsKeys::TIMER_RESUME_AFTER_BREAK_DEFAULT)
          .toBool();
  const bool oldReset =
      settings
          ->value(SettingsKeys::TIMER_RESET_ON_TASK_SWITCH,
                  SettingsKeys::TIMER_RESET_ON_TASK_SWITCH_DEFAULT)
          .toBool();

  settings->setValue(SettingsKeys::TIMER_TASK_DURATION,
                     taskDurationSpinBox->value());
  settings->setValue(SettingsKeys::TIMER_BREAK_DURATION,
                     breakDurationSpinBox->value());
  settings->setValue(SettingsKeys::TIMER_RESUME_AFTER_BREAK,
                     resumeAfterBreakCheckBox->isChecked());
  settings->setValue(SettingsKeys::TIMER_RESET_ON_TASK_SWITCH,
                     resetOnSwitchCheckBox->isChecked());

  m_timerSettingsChanged =
      oldTaskDuration != taskDurationSpinBox->value() ||
      oldBreakDuration != breakDurationSpinBox->value() ||
      oldResume != resumeAfterBreakCheckBox->isChecked() ||
      oldReset != resetOnSwitchCheckBox->isChecked();
}
