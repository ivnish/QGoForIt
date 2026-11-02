#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Colors {
constexpr auto CHECKBOX_BORDER = "#adaeaf";
constexpr auto CHECKBOX_CHECKED = "#adaeaf";
constexpr auto TOOLBAR_BORDER = "#bcbebf";
constexpr auto TOOLBAR_BACKGROUND = "#e6e6e6";
constexpr auto BUTTON_INACTIVE = "#fcfcfc";
constexpr auto BUTTON_ACTIVE = "#e6e6e6";
constexpr auto ADD_BUTTON_HOVER = "#444";
constexpr auto DRAG_HANDLE = "#999";
constexpr auto TIMER_BREAK = "#3584e4";
constexpr auto TIMER_WORK = "#333333";
constexpr auto TIMER_UNIT_BORDER = "#adaeaf";
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
inline const QString CHECKBOX_TASK =
    QStringLiteral(
        "QCheckBox { spacing: 0; margin: 0; padding: 0; }"
        "QCheckBox::indicator { width: %1px; height: %1px; border-radius: %2px;"
        "  border: 1px solid %3; background: white; }"
        "QCheckBox::indicator:checked { background: %4; }")
        .arg(Sizes::INDICATOR_SIZE)
        .arg(Sizes::INDICATOR_RADIUS)
        .arg(Colors::CHECKBOX_BORDER)
        .arg(Colors::CHECKBOX_CHECKED);

inline const QString CHECKBOX_DONE =
    QStringLiteral(
        "QCheckBox { spacing: 0; margin: 0; padding: 0; }"
        "QCheckBox::indicator { width: %1px; height: %1px; border-radius: %2px;"
        "  border: 1px solid %3; background: %4; }"
        "QCheckBox::indicator:checked { background: %4;"
        "  image: url(:/icons/check.svg); }")
        .arg(Sizes::INDICATOR_SIZE)
        .arg(Sizes::INDICATOR_RADIUS)
        .arg(Colors::CHECKBOX_BORDER)
        .arg(Colors::CHECKBOX_CHECKED);

inline const QString BUTTON_TODO =
    QStringLiteral(
        "QToolButton { margin:0; padding:0 8px; background:%1;"
        "  border:1px solid %2;"
        "  border-top-left-radius:4px; border-bottom-left-radius:4px;"
        "  border-top-right-radius:0; border-bottom-right-radius:0; }"
        "QToolButton:checked { background:%3; }")
        .arg(Colors::BUTTON_INACTIVE)
        .arg(Colors::TOOLBAR_BORDER)
        .arg(Colors::BUTTON_ACTIVE);

inline const QString BUTTON_TIMER =
    QStringLiteral(
        "QToolButton { margin:0; padding:0 8px; background:%1;"
        "  border:1px solid %2; border-left:none; border-radius:0; }"
        "QToolButton:checked { background:%3; }"
        "QToolButton:disabled { background:%1; color: inherit; }")
        .arg(Colors::BUTTON_INACTIVE)
        .arg(Colors::TOOLBAR_BORDER)
        .arg(Colors::BUTTON_ACTIVE);

inline const QString BUTTON_DONE =
    QStringLiteral(
        "QToolButton { margin:0; padding:0 8px; background:%1;"
        "  border:1px solid %2; border-left:none;"
        "  border-top-right-radius:4px; border-bottom-right-radius:4px;"
        "  border-top-left-radius:0; border-bottom-left-radius:0; }"
        "QToolButton:checked { background:%3; }")
        .arg(Colors::BUTTON_INACTIVE)
        .arg(Colors::TOOLBAR_BORDER)
        .arg(Colors::BUTTON_ACTIVE);

inline const QString TOOLBAR =
    QStringLiteral(
        "#toolbarContainer { background: %1; border-bottom: 1px solid %2; }")
        .arg(Colors::TOOLBAR_BACKGROUND)
        .arg(Colors::TOOLBAR_BORDER);

inline const QString INPUT_CONTAINER =
    QStringLiteral(
        "#taskInputContainer { border: 1px solid %1; border-radius: 4px;"
        "  background: white; }")
        .arg(Colors::CHECKBOX_BORDER);

inline const QString ADD_BUTTON =
    QStringLiteral(
        "QToolButton { border: none; background: transparent;"
        "  font-size: 16px; font-weight: bold; color: black; }"
        "QToolButton:hover { color: %1; }")
        .arg(Colors::ADD_BUTTON_HOVER);

inline const QString TIMER_UNIT =
    QStringLiteral(
        "#timerUnitFrame { border: 1px solid %1; border-radius: 6px;"
        "  background: white; }"
        "QSpinBox { border: none; background: transparent; }"
        "QToolButton { border: 1px solid %1; border-radius: 3px;"
        "  background: %5; font-size: 13px; font-weight: bold; color: %2; }"
        "QToolButton:hover { background: %3; }"
        "QToolButton:pressed { background: %4; }"
        "QToolButton:disabled { color: #ccc; background: %5; }")
        .arg(Colors::TIMER_UNIT_BORDER)
        .arg(Colors::TIMER_WORK)
        .arg(Colors::BUTTON_ACTIVE)
        .arg(Colors::TOOLBAR_BORDER)
        .arg(Colors::BUTTON_INACTIVE);

inline const QString TIMER_BUTTON =
    QStringLiteral(
        "QPushButton { margin:0; padding:0 12px; background:%1;"
        "  border:1px solid %2; border-radius:4px; }"
        "QPushButton:hover { background:%3; }"
        "QPushButton:pressed { background:%4; }"
        "QPushButton:disabled { color:#aaa; }")
        .arg(Colors::BUTTON_INACTIVE)
        .arg(Colors::TOOLBAR_BORDER)
        .arg(Colors::BUTTON_ACTIVE)
        .arg(Colors::TOOLBAR_BORDER);

// Format string: %1 = trough color, %2 = chunk color (phase-dependent).
inline const QString TIMER_PROGRESS =
    QStringLiteral(
        "QProgressBar { border:none; background:%1; border-radius:3px;"
        "  text-align:center; }"
        "QProgressBar::chunk { background:%2; border-radius:3px; }");
}

#endif  // CONSTANTS_H
