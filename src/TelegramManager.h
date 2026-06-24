#pragma once

#include <QString>

/// Инкапсулирует логику работы с Telegram: проверка процесса, регистрация схемы.
class TelegramManager {
public:
    /// Проверить, запущен ли процесс Telegram Desktop (кроссплатформенно)
    static bool isProcessRunning();

    /// Проверить, зарегистрирован ли обработчик tg:// протокола
    static bool isSchemeRegistered();
};
