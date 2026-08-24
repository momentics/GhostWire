#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "TelegramManager.h"
#include "Config.h"
#ifdef Q_OS_MAC
#include "MacPlatformIntegration.h"
#endif
#include <QProcess>
#include <QString>

/// Проверить, запущен ли процесс Telegram Desktop (кроссплатформенно)
bool TelegramManager::isProcessRunning() {
#ifdef Q_OS_MAC
    return MacPlatformIntegration::isTelegramRunning();
#else
    QProcess proc;

#ifdef _WIN32
    proc.start("tasklist", QStringList() << "/FI"
        << QString("IMAGENAME eq %1").arg(Config::TELEGRAM_PROCESS_NAME)
        << "/NH");
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
#elif defined(Q_OS_MAC)
    return MacPlatformIntegration::isUrlSchemeRegistered(QStringLiteral("tg://socks"));
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
