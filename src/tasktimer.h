#ifndef TASKTIMER_H
#define TASKTIMER_H

#include <QElapsedTimer>
#include <QObject>

class QSettings;
class QTimer;

/**
 * Countdown timer core with an explicit state machine, decoupled from the UI.
 *
 * Time tracking is based on monotonic clock timestamps, so the update
 * interval has no influence on accuracy. A jump of the monotonic clock
 * larger than TIMER_SUSPEND_THRESHOLD_MS between two ticks means the
 * system was suspended (or the process was starved): the countdown is
 * then paused, preserving the last consistent remaining time.
 */
class TaskTimer : public QObject {
  Q_OBJECT

 public:
  enum class State { Idle, Running, Paused, Completed };
  enum class Phase { Work, Break };

  explicit TaskTimer(QSettings *settings, QObject *parent = nullptr);

  State state() const { return m_state; }
  Phase phase() const { return m_phase; }
  QString activeTask() const { return m_activeTask; }
  bool hasActiveTask() const { return !m_activeTask.isEmpty(); }

  // Remaining time of the current iteration; never negative.
  qint64 remainingMs() const;
  // Total duration of the current iteration (configured phase duration).
  qint64 iterationDurationMs() const { return m_iterationDurationMs; }
  // Elapsed fraction of the current iteration, 0.0 .. 1.0.
  double progress() const;

  // Tasks are identified by their text: selecting a task with the same
  // text as the current one is a no-op, and clearing the selection in
  // the To-Do list (e.g. after an external todo.txt reload) pauses the
  // countdown.
  void setActiveTask(const QString &taskName);
  // Duration changes apply from the next reset, not to a running countdown.
  void setDurations(qint64 taskMs, qint64 breakMs);
  // In Idle (and Paused) redefines the remaining time of the iteration.
  void setRemainingMs(qint64 ms);
  // Re-read settings; running countdown is not affected.
  void reloadSettings();

 public slots:
  void start();
  void pause();
  void resume();
  void toggleRunning();
  // Reset the current phase to its configured duration, state -> Idle.
  void reset();
  // Finish the current iteration immediately, moving to the next phase.
  void skip();
  // Recompute remaining time and emit update signals. Called by the tick.
  void update();

 signals:
  void stateChanged(TaskTimer::State newState);
  void phaseChanged(TaskTimer::Phase newPhase);
  void activeTaskChanged(const QString &taskName);
  void remainingChanged(qint64 remainingMs);
  void progressChanged(double progress);
  // Emitted exactly once per completed iteration; |skipped| is true when
  // the phase ended through skip() instead of the countdown reaching zero.
  void iterationFinished(TaskTimer::Phase finishedPhase, bool skipped);
  // The countdown was paused because a system sleep/freeze was detected.
  void suspendedDetected();

 private:
  void endIteration(bool skipped);
  void toggleBreak();
  void setState(State newState);
  void stopInternal(qint64 lastMeasurementMs);
  qint64 phaseDurationMs(Phase phase) const;
  void emitUpdated();

  QSettings *m_settings = nullptr;

  State m_state = State::Idle;
  Phase m_phase = Phase::Work;
  QString m_activeTask;

  qint64 m_iterationDurationMs = 0;
  qint64 m_previousRuntimeMs = 0;
  qint64 m_taskDurationMs = 0;
  qint64 m_breakDurationMs = 0;
  bool m_resumeAfterBreak = true;
  bool m_resetOnTaskSwitch = true;

  QElapsedTimer m_runClock;  // monotonic clock of the current run
  bool m_runClockValid = false;
  qint64 m_prevTickMs = 0;

  QTimer *m_tickTimer = nullptr;
};

#endif  // TASKTIMER_H
