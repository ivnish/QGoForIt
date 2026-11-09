#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "utils.h"

#include <QString>

namespace Colors {
// Brand accent; lightness is adjusted by Theme::resolve for dark schemes.
constexpr auto TIMER_BREAK = "#3584e4";
}

namespace Sizes {
constexpr int ROW_HEIGHT = 26;
constexpr int ROW_MARGINS_TOTAL = 8;
constexpr int CHECKBOX_SIZE = 16;
constexpr int INDICATOR_SIZE = 14;
constexpr int INDICATOR_RADIUS = 1;
constexpr int DRAG_HANDLE_WIDTH = 24;
constexpr int DRAG_HANDLE_FONT_SIZE = 18;
constexpr int GAP_HEIGHT = 26;
constexpr int SCROLL_DISTANCE = 30;
constexpr int SCROLL_STEP = 8;
constexpr int SCROLL_INTERVAL = 50;
constexpr int DRAG_TIMER_INTERVAL = 16;
constexpr int WATCHER_DEBOUNCE_MS = 200;
constexpr int TIMER_TICK_MS = 250;
constexpr int TIMER_SUSPEND_THRESHOLD_MS = 60 * 1000;
// Font scales are relative to the application base font, matching
// GoForIt's CSS (spinbutton 400%, label 250%, task-status 200%,
// task-active/break 120%).
constexpr double TIMER_UNIT_FONT_SCALE = 4.0;
constexpr double TIMER_SEPARATOR_FONT_SCALE = 2.5;
constexpr double TIMER_STATUS_FONT_SCALE = 2.0;
constexpr double TIMER_TASK_FONT_SCALE = 1.2;
constexpr int TIMER_UNIT_BUTTON_SIZE = 20;
constexpr int TIMER_UNIT_PADDING = 8;
constexpr int TIMER_UNIT_MARGIN_H = 4;
constexpr int TIMER_UNIT_MARGIN_V = 2;
constexpr int TIMER_DISPLAY_SPACING = 2;
constexpr int TIMER_PAGE_MARGIN = 16;
constexpr int TIMER_BUTTON_MIN_WIDTH = 64;
constexpr int TIMER_BUTTON_HEIGHT = 32;
constexpr int TIMER_PROGRESS_HEIGHT = 6;
constexpr int TIMER_BLINK_INTERVAL_MS = 250;
constexpr int TIMER_BLINK_COUNT = 6;
constexpr int NOTIFICATION_TIMEOUT_MS = 5000;
constexpr int DROP_GAP_ALPHA = 55;
}

namespace Formats {
constexpr auto DATE_FORMAT = "dd.MM.yyyy";
constexpr auto DONE_PREFIX = "x ";
}

namespace SettingsKeys {
constexpr auto FILE_WATCHER = "settings/fileWatcher";
constexpr auto FILE_WATCHER_DEFAULT = false;
constexpr auto AUTO_CLEANUP = "settings/autoCleanup";
constexpr auto AUTO_CLEANUP_DEFAULT = false;
constexpr auto CLEANUP_DAYS = "settings/cleanupDays";
constexpr int CLEANUP_DAYS_DEFAULT = 7;
constexpr int CLEANUP_DAYS_MIN = 1;
constexpr int CLEANUP_DAYS_MAX = 365;
constexpr auto TIMER_TASK_DURATION = "settings/timerTaskDuration";
constexpr int TIMER_TASK_DURATION_DEFAULT = 25;  // minutes
constexpr auto TIMER_BREAK_DURATION = "settings/timerBreakDuration";
constexpr int TIMER_BREAK_DURATION_DEFAULT = 5;  // minutes
constexpr int TIMER_DURATION_MIN = 1;            // minutes
constexpr int TIMER_DURATION_MAX = 1439;         // minutes
constexpr auto TIMER_RESUME_AFTER_BREAK = "settings/timerResumeAfterBreak";
constexpr auto TIMER_RESUME_AFTER_BREAK_DEFAULT = true;
constexpr auto TIMER_RESET_ON_TASK_SWITCH = "settings/timerResetOnTaskSwitch";
constexpr auto TIMER_RESET_ON_TASK_SWITCH_DEFAULT = true;
}

namespace Styles {
inline QString checkboxTask(const Theme::Palette &t) {
  return QStringLiteral(
             "QCheckBox { spacing: 0; margin: 0; padding: 0; color: %1; }"
             "QCheckBox::indicator { width: %2px; height: %2px;"
             "  border-radius: %3px; border: 1px solid %4;"
             "  background: %5; }"
             "QCheckBox::indicator:checked { background: %6; }")
      .arg(t.text.name())
      .arg(Sizes::INDICATOR_SIZE)
      .arg(Sizes::INDICATOR_RADIUS)
      .arg(t.mid.name())
      .arg(t.base.name())
      .arg(t.mid.name());
}

inline QString checkboxDone(const Theme::Palette &t) {
  return QStringLiteral(
             "QCheckBox { spacing: 0; margin: 0; padding: 0; color: %1; }"
             "QCheckBox::indicator { width: %2px; height: %2px;"
             "  border-radius: %3px; border: 1px solid %4;"
             "  background: %5; }"
             "QCheckBox::indicator:checked { background: %5;"
             "  image: url(:/icons/check.svg); }")
      .arg(t.text.name())
      .arg(Sizes::INDICATOR_SIZE)
      .arg(Sizes::INDICATOR_RADIUS)
      .arg(t.mid.name())
      .arg(t.mid.name());
}

inline QString buttonTodo(const Theme::Palette &t) {
  return QStringLiteral(
             "QToolButton { margin:0; padding:0 8px; background:%1;"
             "  border:1px solid %2; color:%3;"
             "  border-top-left-radius:4px; border-bottom-left-radius:4px;"
             "  border-top-right-radius:0; border-bottom-right-radius:0; }"
             "QToolButton:checked { background:%4; }")
      .arg(t.button.name(), t.mid.name(), t.text.name(), t.active.name());
}

inline QString buttonTimer(const Theme::Palette &t) {
  return QStringLiteral(
             "QToolButton { margin:0; padding:0 8px; background:%1;"
             "  border:1px solid %2; border-left:none; border-radius:0;"
             "  color:%3; }"
             "QToolButton:checked { background:%4; }"
             "QToolButton:disabled { background:%1; color:%5; }")
      .arg(t.button.name(), t.mid.name(), t.text.name(), t.active.name(),
           t.dim.name());
}

inline QString buttonDone(const Theme::Palette &t) {
  return QStringLiteral(
             "QToolButton { margin:0; padding:0 8px; background:%1;"
             "  border:1px solid %2; border-left:none; color:%3;"
             "  border-top-right-radius:4px; border-bottom-right-radius:4px;"
             "  border-top-left-radius:0; border-bottom-left-radius:0; }"
             "QToolButton:checked { background:%4; }")
      .arg(t.button.name(), t.mid.name(), t.text.name(), t.active.name());
}

// Strip background: t.window by default, dominantMenuBarColor() override.
inline QString toolbar(const Theme::Palette &t, const QColor &strip) {
  return QStringLiteral(
             "#toolbarContainer { background: %1;"
             "  border-bottom: 1px solid %2; }")
      .arg(strip.name(), t.mid.name());
}

inline QString inputContainer(const Theme::Palette &t) {
  return QStringLiteral(
             "#taskInputContainer { border: 1px solid %1; border-radius: 4px;"
             "  background: %2; }")
      .arg(t.mid.name(), t.base.name());
}

inline QString addButton(const Theme::Palette &t) {
  return QStringLiteral(
             "QToolButton { border: none; background: transparent;"
             "  font-size: 16px; font-weight: bold; color: %1; }"
             "QToolButton:hover { color: %2; }")
      .arg(t.text.name(), t.dim.name());
}

inline QString timerUnit(const Theme::Palette &t) {
  return QStringLiteral(
             "#timerUnitFrame { border: 1px solid %1; border-radius: 6px;"
             "  background: %2; }"
             "QSpinBox { border: none; background: transparent;"
             "  color: %3; }"
             "QToolButton { border: 1px solid %1; border-radius: 3px;"
             "  background: %4; font-size: 13px; font-weight: bold;"
             "  color: %3; }"
             "QToolButton:hover { background: %5; }"
             "QToolButton:pressed { background: %6; }"
             "QToolButton:disabled { color: %7; background: %4; }")
      .arg(t.mid.name(), t.base.name(), t.text.name(), t.button.name(),
           t.hover.name(), t.active.name(), t.dim.name());
}

inline QString timerButton(const Theme::Palette &t) {
  return QStringLiteral(
             "QPushButton { margin:0; padding:0 12px; background:%1;"
             "  border:1px solid %2; border-radius:4px; color:%3; }"
             "QPushButton:hover { background:%4; }"
             "QPushButton:pressed { background:%5; }"
             "QPushButton:disabled { color:%6; }")
      .arg(t.button.name(), t.mid.name(), t.text.name(), t.hover.name(),
           t.active.name(), t.dim.name());
}

// Format string: chunk color is phase-dependent.
inline QString timerProgress(const Theme::Palette &t, const QColor &chunk) {
  return QStringLiteral(
             "QProgressBar { border:none; background:%1; border-radius:3px;"
             "  text-align:center; }"
             "QProgressBar::chunk { background:%2; border-radius:3px; }")
      .arg(t.trough.name(), chunk.name());
}
}

#endif  // CONSTANTS_H
