#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "TelegramManager.h"
#include "Config.h"
#include <QProcess>
#include <QString>

/// Проверить, запущен ли процесс Telegram Desktop (кроссплатформенно)
bool TelegramManager::isProcessRunning() {
    QProcess proc;

#ifdef _WIN32
    proc.start("tasklist", QStringList() << "/FI"
        << QString("IMAGENAME eq %1").arg(Config::TELEGRAM_PROCESS_NAME)
        << "/NH");
#elif defined(__APPLE__)
    proc.start("pgrep", QStringList() << "-i" << Config::TELEGRAM_PROCESS_NAME);
#else
    // Linux: pgrep с case-insensitive по частичному имени «telegram»
    // ловит telegram-desktop, telegram-desktop-bin и т.д.
    proc.start("pgrep", QStringList() << "-i" << "telegram");
#endif

    proc.waitForFinished(3000);
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

#ifdef _WIN32
    return output.contains(Config::TELEGRAM_PROCESS_NAME, Qt::CaseInsensitive);
#else
    // pgrep возвращает PID (число) если процесс найден, пусто если нет
    return !output.trimmed().isEmpty();
#endif
}

/// Проверить, зарегистрирован ли обработчик tg:// протокола
bool TelegramManager::isSchemeRegistered() {
#ifdef _WIN32
    // Проверяем реестр: HKEY_CLASSES_ROOT/tg/
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"tg", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
#elif defined(__APPLE__)
    // macOS: через LaunchServices проверяем, есть ли приложение для tg://
    QProcess proc;
    proc.start("osascript", QStringList()
        << "-e" << "on run {url}"
        << "-e" << "tell application \"System Events\" to get (name of processes whose name is \"Telegram\")"
        << "-e" << "end run"
        << "tg://");
    proc.waitForFinished(2000);
    // Если Telegram запущен — handler точно есть
    if (proc.exitCode() == 0 && !proc.readAllStandardOutput().trimmed().isEmpty())
        return true;
    // Иначе проверяем через launchctl: есть ли приложение, обрабатывающее tg://
    QProcess lsProc;
    lsProc.start("sh", QStringList() << "-c"
        << "lsregister -dump 2>/dev/null | grep -qi 'tg:' && echo yes");
    lsProc.waitForFinished(2000);
    return lsProc.readAllStandardOutput().trimmed() == "yes";
#else
    // Linux: через xdg-settings проверяем handler для tg://
    QProcess proc;
    proc.start("xdg-settings", QStringList()
        << "get" << "default-url-scheme-handler" << "tg");
    proc.waitForFinished(2000);
    QString handler = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    return !handler.isEmpty() && !handler.contains("not found", Qt::CaseInsensitive);
#endif
}
