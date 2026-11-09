#ifndef TIMERWIDGET_H
#define TIMERWIDGET_H

#include "tasktimer.h"
#include "utils.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QProgressBar;
class QSpinBox;
class QToolButton;
class QTimer;
class QSettings;

/**
 * Widget for displaying and editing one unit (hours/minutes/seconds) of
 * the countdown, laid out like GoForIt's vertical spin buttons:
 * the increase button sits above the digits, the decrease button below.
 * The digits are a QSpinBox, so the value can also be edited with the
 * keyboard (digits, Up/Down arrows) and the mouse wheel.
 */
class TimeUnitWidget : public QWidget {
  Q_OBJECT

 public:
  explicit TimeUnitWidget(const QString &unitName, QWidget *parent = nullptr);

  int value() const;
  void setValue(int value);
  void setAdjustable(bool adjustable);
  void setDigitsDim(bool dim);
  void retheme(const Theme::Palette &t);

 signals:
  void valueEdited();

 private:
  Theme::Palette curTheme;
  QSpinBox *spin;
  QToolButton *upButton;
  QToolButton *downButton;
};

/**
 * The "Timer" page: active task, countdown display, progress bar and
 * Start/Pause/Resume, Skip and Reset controls.
 */
class TimerWidget : public QWidget {
  Q_OBJECT

 public:
  explicit TimerWidget(QSettings *settings, QWidget *parent = nullptr);

  void setActiveTask(const QString &taskName);
  // Re-reads timer settings; affects the idle timer immediately and the
  // running countdown from its next reset.
  void reloadSettings();
  void retheme(const Theme::Palette &t);

 private:
  TaskTimer *timer;
  class Notifier *notifier;

  QLabel *statusLabel;
  QLabel *taskLabel;
  Theme::Palette curTheme;
  TimeUnitWidget *hoursUnit;
  TimeUnitWidget *minutesUnit;
  TimeUnitWidget *secondsUnit;
  QProgressBar *progressBar;
  QPushButton *resetButton;
  QPushButton *skipButton;
  QPushButton *mainButton;
  QWidget *actionRow;
  QWidget *buttonsRow;
  QTimer *blinkTimer;
  int blinkCountdown = 0;

  void setupUi();
  void connectTimer();
  void applyRemaining(qint64 remainingMs);
  void onUnitEdited();
  bool unitsAdjustable() const;
  void refreshStateControls();
  void refreshTaskHeader();
  void refreshPhaseHeader();
  void startBlinking();
  void blinkTick();
};

#endif  // TIMERWIDGET_H
