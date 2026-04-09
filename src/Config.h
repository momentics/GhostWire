#pragma once

#include <chrono>

namespace Config {

/// Интервал опроса статистики библиотеки (мс)
inline constexpr int STATS_POLL_INTERVAL_MS = 500;

/// Максимальное количество точек на спарклайне (30 минут / 0.5с = 3600)
inline constexpr int SPARKLINE_MAX_POINTS = 3600;

/// Имя файла конфигурации для библиотеки GhostWire
inline const char* LIB_CONFIG_RESOURCE = ":/config.json";

/// Ресурсы иконок трея
inline const char* TRAY_ICON_IDLE     = ":/icons/tray_idle.png";
inline const char* TRAY_ICON_ACTIVE   = ":/icons/tray_active.png";

/// Имена кадров анимации (покадровая)
inline const char* TRAY_ANIM_FRAME_PATTERN = ":/icons/tray_active_frames/frame_%1.png";
inline constexpr int TRAY_ANIM_FRAME_COUNT  = 6;

/// Размер иконки для трея (Windows: 16x16, DPI-aware — берётся системный)
inline constexpr int TRAY_ICON_SIZE = 16;

/// Размер popup-меню
inline constexpr int MENU_WIDTH  = 280;

/// Максимальное количество точек данных для расчёта дельты RX/TX
inline constexpr int STATS_HISTORY_SIZE = 500;

} // namespace Config
