#pragma once

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
inline constexpr int MENU_WIDTH  = 240;

/// Порт SOCKS5 прокси
inline constexpr int SOCKS_PORT = 1080;

/// Адрес SOCKS5 прокси
inline const char* SOCKS_SERVER = "127.0.0.1";

/// Имя процесса Telegram Desktop
#ifdef _WIN32
inline const char* TELEGRAM_PROCESS_NAME = "Telegram.exe";
#elif defined(__APPLE__)
inline const char* TELEGRAM_PROCESS_NAME = "Telegram";
#else
inline const char* TELEGRAM_PROCESS_NAME = "telegram-desktop";
#endif

} // namespace Config
