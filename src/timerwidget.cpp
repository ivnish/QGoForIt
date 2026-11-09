#include "timerwidget.h"
#include "constants.h"
#include "notifier.h"
#include "utils.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
// QSpinBox has no leading-zero formatting of its own.
class UnitSpinBox : public QSpinBox {
 public:
  using QSpinBox::QSpinBox;

 protected:
  QString textFromValue(int value) const override {
    return QString::number(value).rightJustified(2, QLatin1Char('0'));
  }

  // The base class selects the whole text after stepping (keyboard
  // arrows, wheel, PageUp/PageDown); drop that selection so the digits
  // keep a clean look.
  void stepBy(int steps) override {
    QSpinBox::stepBy(steps);
    lineEdit()->deselect();
    lineEdit()->setCursorPosition(lineEdit()->text().size());
  }
};
}  // namespace

TimeUnitWidget::TimeUnitWidget(const QString &unitName, QWidget *parent)
    : QWidget(parent) {
  setObjectName("timerUnitFrame");

  QFont unitFont = QFontDatabase().systemFont(QFontDatabase::FixedFont);
  unitFont.setPointSizeF(
      qMax<qreal>(1.0, qApp->font().pointSizeF() * Sizes::TIMER_UNIT_FONT_SCALE));

  spin = new UnitSpinBox(this);
  spin->setRange(0, 59);
  spin->setAlignment(Qt::AlignCenter);
  spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
  spin->setFrame(false);
  spin->setFont(unitFont);
  spin->setReadOnly(true);
  spin->setKeyboardTracking(false);
  spin->setAccessibleName(unitName);
  spin->setToolTip(unitName);

  const int digitsWidth =
      QFontMetrics(unitFont).horizontalAdvance(QStringLiteral("00")) +
      Sizes::TIMER_UNIT_PADDING;
  spin->setFixedWidth(digitsWidth);

  upButton = new QToolButton(this);
  upButton->setText(QStringLiteral("+"));
  upButton->setFixedSize(Sizes::TIMER_UNIT_BUTTON_SIZE,
                         Sizes::TIMER_UNIT_BUTTON_SIZE);
  upButton->setAutoRepeat(true);
  upButton->setAutoRepeatDelay(250);
  upButton->setAutoRepeatInterval(50);
  upButton->setAccessibleName(tr("Increase %1").arg(unitName));
  upButton->setToolTip(tr("Increase %1").arg(unitName));

  downButton = new QToolButton(this);
  downButton->setText(QStringLiteral("\u2212"));
  downButton->setFixedSize(Sizes::TIMER_UNIT_BUTTON_SIZE,
                           Sizes::TIMER_UNIT_BUTTON_SIZE);
  downButton->setAutoRepeat(true);
  downButton->setAutoRepeatDelay(250);
  downButton->setAutoRepeatInterval(50);
  downButton->setAccessibleName(tr("Decrease %1").arg(unitName));
  downButton->setToolTip(tr("Decrease %1").arg(unitName));

  connect(upButton, &QToolButton::clicked, this,
          [this]() { spin->setValue(spin->value() + 1); });
  connect(downButton, &QToolButton::clicked, this,
          [this]() { spin->setValue(spin->value() - 1); });
  connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &TimeUnitWidget::valueEdited);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(Sizes::TIMER_UNIT_MARGIN_H, Sizes::TIMER_UNIT_MARGIN_V,
                             Sizes::TIMER_UNIT_MARGIN_H, Sizes::TIMER_UNIT_MARGIN_V);
  layout->setSpacing(0);
  layout->addWidget(upButton, 0, Qt::AlignHCenter);
  layout->addWidget(spin, 0, Qt::AlignHCenter);
  layout->addWidget(downButton, 0, Qt::AlignHCenter);
}

int TimeUnitWidget::value() const { return spin->value(); }

void TimeUnitWidget::setValue(int value) { spin->setValue(value); }

void TimeUnitWidget::setAdjustable(bool adjustable) {
  spin->setReadOnly(!adjustable);
  upButton->setEnabled(adjustable);
  downButton->setEnabled(adjustable);
}

void TimeUnitWidget::setDigitsDim(bool dim) {
  spin->setStyleSheet(
      dim ? QStringLiteral("color: %1;").arg(curTheme.dim.name()) : QString());
}

void TimeUnitWidget::retheme(const Theme::Palette &t) {
  curTheme = t;
  setStyleSheet(Styles::timerUnit(t));
}

TimerWidget::TimerWidget(QSettings *settings, QWidget *parent)
    : QWidget(parent) {
  timer = new TaskTimer(settings, this);
  notifier = new Notifier(this);

  setupUi();
  connectTimer();

  // The theme must be cached before any refresh* call below.
  curTheme = Theme::resolve(QApplication::palette());
  refreshTaskHeader();
  refreshPhaseHeader();
  refreshStateControls();
  applyRemaining(timer->remainingMs());
  retheme(curTheme);
}

void TimerWidget::retheme(const Theme::Palette &t) {
  curTheme = t;
  hoursUnit->retheme(t);
  minutesUnit->retheme(t);
  secondsUnit->retheme(t);
  resetButton->setStyleSheet(Styles::timerButton(t));
  skipButton->setStyleSheet(Styles::timerButton(t));
  mainButton->setStyleSheet(Styles::timerButton(t));
  refreshPhaseHeader();
}

void TimerWidget::setupUi() {
  // Match the task list background via roles, so the page follows the
  // active palette without hardcoding.
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  auto scaledFont = [](double scale) {
    QFont f = qApp->font();
    f.setPointSizeF(qMax<qreal>(1.0, f.pointSizeF() * scale));
    return f;
  };

  statusLabel = new QLabel(this);
  QFont statusFont = scaledFont(Sizes::TIMER_STATUS_FONT_SCALE);
  statusFont.setBold(true);
  statusLabel->setFont(statusFont);
  statusLabel->setAlignment(Qt::AlignCenter);

  taskLabel = new QLabel(this);
  taskLabel->setFont(scaledFont(Sizes::TIMER_TASK_FONT_SCALE));
  taskLabel->setAlignment(Qt::AlignCenter);
  taskLabel->setWordWrap(true);

  hoursUnit = new TimeUnitWidget(tr("hours"), this);
  minutesUnit = new TimeUnitWidget(tr("minutes"), this);
  secondsUnit = new TimeUnitWidget(tr("seconds"), this);

  const QFont separatorFont = scaledFont(Sizes::TIMER_SEPARATOR_FONT_SCALE);
  auto *separator1 = new QLabel(QStringLiteral(":"), this);
  separator1->setFont(separatorFont);
  separator1->setAlignment(Qt::AlignCenter);

  auto *separator2 = new QLabel(QStringLiteral(":"), this);
  separator2->setFont(separatorFont);
  separator2->setAlignment(Qt::AlignCenter);

  auto *displayLayout = new QHBoxLayout;
  displayLayout->setSpacing(Sizes::TIMER_DISPLAY_SPACING);
  displayLayout->addStretch(1);
  displayLayout->addWidget(hoursUnit);
  displayLayout->addWidget(separator1);
  displayLayout->addWidget(minutesUnit);
  displayLayout->addWidget(separator2);
  displayLayout->addWidget(secondsUnit);
  displayLayout->addStretch(1);

  progressBar = new QProgressBar(this);
  progressBar->setTextVisible(false);
  progressBar->setFixedHeight(Sizes::TIMER_PROGRESS_HEIGHT);
  progressBar->setRange(0, 1000);
  progressBar->setValue(0);
  progressBar->setAccessibleName(tr("Timer progress"));

  resetButton = new QPushButton(tr("Reset"), this);
  resetButton->setMinimumHeight(Sizes::TIMER_BUTTON_HEIGHT);
  resetButton->setAccessibleName(tr("Reset the timer"));
  connect(resetButton, &QPushButton::clicked, timer, &TaskTimer::reset);

  skipButton = new QPushButton(tr("Skip"), this);
  skipButton->setMinimumHeight(Sizes::TIMER_BUTTON_HEIGHT);
  skipButton->setAccessibleName(tr("Skip to the next phase"));
  connect(skipButton, &QPushButton::clicked, timer, &TaskTimer::skip);

  mainButton = new QPushButton(tr("&Start"), this);
  mainButton->setMinimumHeight(Sizes::TIMER_BUTTON_HEIGHT);
  mainButton->setMinimumWidth(Sizes::TIMER_BUTTON_MIN_WIDTH);
  connect(mainButton, &QPushButton::clicked, timer, &TaskTimer::toggleRunning);

  actionRow = new QWidget(this);
  auto *actionLayout = new QHBoxLayout(actionRow);
  actionLayout->setContentsMargins(0, 0, 0, 0);
  actionLayout->setSpacing(8);
  actionLayout->addWidget(resetButton);
  actionLayout->addStretch(1);
  actionLayout->addWidget(skipButton);
  actionLayout->addWidget(mainButton);

  buttonsRow = new QWidget(this);
  auto *buttonsLayout = new QVBoxLayout(buttonsRow);
  buttonsLayout->setContentsMargins(0, 0, 0, 0);
  buttonsLayout->setSpacing(8);
  buttonsLayout->addStretch(1);
  buttonsLayout->addWidget(actionRow);
  buttonsLayout->addStretch(1);

  blinkTimer = new QTimer(this);
  blinkTimer->setInterval(Sizes::TIMER_BLINK_INTERVAL_MS);
  connect(blinkTimer, &QTimer::timeout, this, &TimerWidget::blinkTick);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(Sizes::TIMER_PAGE_MARGIN, 16,
                             Sizes::TIMER_PAGE_MARGIN, 16);
  layout->setSpacing(8);
  layout->addStretch(1);
  layout->addWidget(statusLabel);
  layout->addWidget(taskLabel);
  layout->addSpacing(16);
  layout->addLayout(displayLayout);
  layout->addSpacing(8);
  layout->addWidget(progressBar);
  layout->addSpacing(16);
  layout->addWidget(buttonsRow);
  layout->addStretch(1);
}

void TimerWidget::connectTimer() {
  connect(timer, &TaskTimer::remainingChanged, this,
          &TimerWidget::applyRemaining);

  connect(timer, &TaskTimer::progressChanged, this, [this](double p) {
    progressBar->setValue(qRound(p * 1000.0));
  });

  connect(timer, &TaskTimer::stateChanged, this,
          [this](TaskTimer::State) { refreshStateControls(); });

  connect(timer, &TaskTimer::phaseChanged, this,
          [this](TaskTimer::Phase) { refreshPhaseHeader(); });

  connect(timer, &TaskTimer::activeTaskChanged, this,
          [this](const QString &) {
            refreshTaskHeader();
            refreshPhaseHeader();
            refreshStateControls();
          });

  connect(timer, &TaskTimer::iterationFinished, this,
          [this](TaskTimer::Phase finishedPhase, bool skipped) {
            startBlinking();
            if (skipped) return;
            if (finishedPhase == TaskTimer::Phase::Work) {
              notifier->notify(
                  tr("Take a Break"),
                  tr("Relax and stop thinking about your current task "
                     "for a while"));
            } else {
              notifier->notify(tr("The Break is Over"),
                               tr("Your current task is") + ": " +
                                   timer->activeTask());
            }
          });

  connect(timer, &TaskTimer::suspendedDetected, this, [this]() {
    notifier->notify(tr("Timer Paused"),
                     tr("The system was suspended, so the countdown was "
                        "paused"));
  });

  connect(hoursUnit, &TimeUnitWidget::valueEdited, this,
          &TimerWidget::onUnitEdited);
  connect(minutesUnit, &TimeUnitWidget::valueEdited, this,
          &TimerWidget::onUnitEdited);
  connect(secondsUnit, &TimeUnitWidget::valueEdited, this,
          &TimerWidget::onUnitEdited);
}

void TimerWidget::setActiveTask(const QString &taskName) {
  timer->setActiveTask(taskName);
}

void TimerWidget::reloadSettings() {
  timer->reloadSettings();
  // A duration change in settings applies to the idle timer at once,
  // matching GoForIt's behavior.
  if (timer->state() == TaskTimer::State::Idle) {
    timer->reset();
  }
}

void TimerWidget::applyRemaining(qint64 remainingMs) {
  const qint64 totalSeconds = remainingMs / 1000;
  hoursUnit->setValue(static_cast<int>(totalSeconds / 3600));
  minutesUnit->setValue(static_cast<int>((totalSeconds / 60) % 60));
  secondsUnit->setValue(static_cast<int>(totalSeconds % 60));
}

bool TimerWidget::unitsAdjustable() const {
  const bool stopped = timer->state() == TaskTimer::State::Idle ||
                       timer->state() == TaskTimer::State::Paused;
  return timer->hasActiveTask() && stopped &&
         timer->phase() == TaskTimer::Phase::Work;
}

void TimerWidget::onUnitEdited() {
  if (!unitsAdjustable()) return;
  const qint64 totalSeconds = hoursUnit->value() * 3600 +
                              minutesUnit->value() * 60 +
                              secondsUnit->value();
  timer->setRemainingMs(totalSeconds * 1000);
}

void TimerWidget::refreshStateControls() {
  const bool hasTask = timer->hasActiveTask();

  switch (timer->state()) {
    case TaskTimer::State::Idle:
      mainButton->setText(tr("&Start"));
      mainButton->setAccessibleName(tr("Start the timer"));
      break;
    case TaskTimer::State::Running:
      mainButton->setText(tr("&Pause"));
      mainButton->setAccessibleName(tr("Pause the timer"));
      break;
    case TaskTimer::State::Paused:
      mainButton->setText(tr("&Resume"));
      mainButton->setAccessibleName(tr("Resume the timer"));
      break;
    case TaskTimer::State::Completed:
      mainButton->setText(tr("&Start"));
      mainButton->setAccessibleName(tr("Start the timer"));
      break;
  }

  const bool adjustable = unitsAdjustable();
  hoursUnit->setAdjustable(adjustable);
  minutesUnit->setAdjustable(adjustable);
  secondsUnit->setAdjustable(adjustable);

  mainButton->setEnabled(hasTask);
  skipButton->setEnabled(hasTask);
  resetButton->setEnabled(hasTask);
  buttonsRow->setVisible(hasTask);
}

void TimerWidget::refreshTaskHeader() {
  if (timer->hasActiveTask()) {
    taskLabel->setText(timer->activeTask());
  } else {
    taskLabel->setText(tr("You have nothing to do."));
  }
}

void TimerWidget::refreshPhaseHeader() {
  const Theme::Palette &t = curTheme;
  QColor chunkColor = t.work;

  if (!timer->hasActiveTask()) {
    statusLabel->setText(tr("Relax") + ".");
    statusLabel->setStyleSheet(
        QString("color: %1;").arg(t.work.name()));
  } else if (timer->phase() == TaskTimer::Phase::Break) {
    chunkColor = t.rest;
    statusLabel->setText(tr("Take a Break") + "!");
    statusLabel->setStyleSheet(
        QString("color: %1;").arg(t.rest.name()));
  } else {
    statusLabel->setText(tr("Active Task") + ":");
    statusLabel->setStyleSheet(
        QString("color: %1;").arg(t.work.name()));
  }

  progressBar->setStyleSheet(Styles::timerProgress(t, chunkColor));
}

void TimerWidget::startBlinking() {
  if (blinkTimer->isActive()) blinkTimer->stop();
  blinkCountdown = Sizes::TIMER_BLINK_COUNT;
  blinkTick();
  blinkTimer->start();
}

void TimerWidget::blinkTick() {
  if (blinkCountdown <= 0) {
    blinkTimer->stop();
    hoursUnit->setDigitsDim(false);
    minutesUnit->setDigitsDim(false);
    secondsUnit->setDigitsDim(false);
    return;
  }

  const bool dim = (blinkCountdown % 2 == 0);
  hoursUnit->setDigitsDim(dim);
  minutesUnit->setDigitsDim(dim);
  secondsUnit->setDigitsDim(dim);
  --blinkCountdown;
}
