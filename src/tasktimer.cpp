#include "tasktimer.h"
#include "constants.h"

#include <QSettings>
#include <QTimer>

TaskTimer::TaskTimer(QSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {
  m_tickTimer = new QTimer(this);
  m_tickTimer->setInterval(Sizes::TIMER_TICK_MS);
  connect(m_tickTimer, &QTimer::timeout, this, &TaskTimer::update);

  reloadSettings();
  m_iterationDurationMs = phaseDurationMs(m_phase);
  emitUpdated();
}

qint64 TaskTimer::remainingMs() const {
  qint64 totalRuntimeMs = m_previousRuntimeMs;
  if (m_state == State::Running && m_runClockValid) {
    totalRuntimeMs += m_runClock.elapsed();
  }
  return qMax<qint64>(0, m_iterationDurationMs - totalRuntimeMs);
}

double TaskTimer::progress() const {
  if (m_iterationDurationMs <= 0) return 1.0;
  const qint64 remaining = remainingMs();
  return 1.0 - static_cast<double>(remaining) /
                   static_cast<double>(m_iterationDurationMs);
}

void TaskTimer::setActiveTask(const QString &taskName) {
  if (m_activeTask == taskName) return;

  // Any running countdown belongs to the previous task: pause it first
  // so that it can never keep ticking for the new one.
  pause();

  if (taskName.isEmpty()) {
    m_activeTask.clear();
    emitUpdated();
    emit activeTaskChanged(QString());
    return;
  }

  if (m_resetOnTaskSwitch) {
    m_phase = Phase::Work;
    emit phaseChanged(m_phase);
    m_iterationDurationMs = phaseDurationMs(m_phase);
    m_previousRuntimeMs = 0;
    setState(State::Idle);
  }

  m_activeTask = taskName;
  emitUpdated();
  emit activeTaskChanged(m_activeTask);
}

void TaskTimer::setDurations(qint64 taskMs, qint64 breakMs) {
  m_taskDurationMs = qMax<qint64>(0, taskMs);
  m_breakDurationMs = qMax<qint64>(0, breakMs);
}

void TaskTimer::setRemainingMs(qint64 ms) {
  if (m_state == State::Running || m_state == State::Completed) return;
  m_iterationDurationMs = m_previousRuntimeMs + qMax<qint64>(0, ms);
  emitUpdated();
}

void TaskTimer::reloadSettings() {
  const int taskMinutes = m_settings
                              ->value(SettingsKeys::TIMER_TASK_DURATION,
                                      SettingsKeys::TIMER_TASK_DURATION_DEFAULT)
                              .toInt();
  const int breakMinutes =
      m_settings
          ->value(SettingsKeys::TIMER_BREAK_DURATION,
                  SettingsKeys::TIMER_BREAK_DURATION_DEFAULT)
          .toInt();
  setDurations(qBound(SettingsKeys::TIMER_DURATION_MIN, taskMinutes,
                      SettingsKeys::TIMER_DURATION_MAX) *
                   60 * 1000,
               qBound(SettingsKeys::TIMER_DURATION_MIN, breakMinutes,
                      SettingsKeys::TIMER_DURATION_MAX) *
                   60 * 1000);
  m_resumeAfterBreak =
      m_settings
          ->value(SettingsKeys::TIMER_RESUME_AFTER_BREAK,
                  SettingsKeys::TIMER_RESUME_AFTER_BREAK_DEFAULT)
          .toBool();
  m_resetOnTaskSwitch =
      m_settings
          ->value(SettingsKeys::TIMER_RESET_ON_TASK_SWITCH,
                  SettingsKeys::TIMER_RESET_ON_TASK_SWITCH_DEFAULT)
          .toBool();
}

void TaskTimer::start() {
  // A zero-length iteration cannot be started: the user cleared the
  // countdown with the spin buttons, let them set a duration first.
  if (m_state == State::Running || !hasActiveTask() ||
      m_iterationDurationMs <= 0) {
    return;
  }

  m_runClock.restart();
  m_runClockValid = true;
  m_prevTickMs = m_runClock.elapsed();
  m_tickTimer->start();
  setState(State::Running);
  emitUpdated();
}

void TaskTimer::pause() {
  if (m_state != State::Running) return;
  stopInternal(m_runClockValid ? m_runClock.elapsed() : 0);
  setState(State::Paused);
  emitUpdated();
}

void TaskTimer::resume() { start(); }

void TaskTimer::toggleRunning() {
  if (m_state == State::Running) {
    pause();
  } else {
    start();
  }
}

void TaskTimer::reset() {
  if (m_state == State::Running) {
    m_tickTimer->stop();
  }
  m_previousRuntimeMs = 0;
  m_runClockValid = false;
  m_iterationDurationMs = phaseDurationMs(m_phase);
  setState(State::Idle);
  emitUpdated();
}

void TaskTimer::skip() {
  if (!hasActiveTask()) return;
  endIteration(true);
}

void TaskTimer::update() {
  if (m_state != State::Running || !m_runClockValid) {
    emitUpdated();
    return;
  }

  const qint64 nowMs = m_runClock.elapsed();
  if (nowMs - m_prevTickMs > Sizes::TIMER_SUSPEND_THRESHOLD_MS) {
    // The monotonic clock jumped: the system was suspended or the
    // process was starved. Pause, preserving the last consistent
    // measurement instead of the (inflated) current one.
    stopInternal(m_prevTickMs);
    setState(State::Paused);
    emit suspendedDetected();
    emitUpdated();
    return;
  }
  m_prevTickMs = nowMs;

  if (remainingMs() <= 0) {
    endIteration(false);
    return;
  }
  emitUpdated();
}

void TaskTimer::endIteration(bool skipped) {
  if (m_state == State::Running) {
    m_tickTimer->stop();
    m_runClockValid = false;
    // Count the phase as fully elapsed.
    m_previousRuntimeMs = m_iterationDurationMs;
  }

  const Phase finishedPhase = m_phase;
  setState(State::Completed);
  emitUpdated();
  emit iterationFinished(finishedPhase, skipped);

  toggleBreak();
}

void TaskTimer::toggleBreak() {
  m_phase = (m_phase == Phase::Work) ? Phase::Break : Phase::Work;
  emit phaseChanged(m_phase);

  m_iterationDurationMs = phaseDurationMs(m_phase);
  m_previousRuntimeMs = 0;
  m_runClockValid = false;
  setState(State::Idle);

  // Breaks always start on their own; after a break the task continues
  // automatically only if the user opted in.
  if (m_phase == Phase::Break || m_resumeAfterBreak) {
    start();
  } else {
    emitUpdated();
  }
}

void TaskTimer::setState(State newState) {
  if (m_state == newState) return;
  m_state = newState;
  emit stateChanged(newState);
}

void TaskTimer::stopInternal(qint64 lastMeasurementMs) {
  m_tickTimer->stop();
  m_previousRuntimeMs += qMax<qint64>(0, lastMeasurementMs);
  m_runClockValid = false;
}

qint64 TaskTimer::phaseDurationMs(Phase phase) const {
  return phase == Phase::Break ? m_breakDurationMs : m_taskDurationMs;
}

void TaskTimer::emitUpdated() {
  emit remainingChanged(remainingMs());
  emit progressChanged(progress());
}
