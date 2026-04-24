#pragma once

#include <QString>
#include <QLibrary>
#include <memory>
#include "../libs/ghostwire/include/ghostwire.h"

class GhostWire {
public:
    GhostWire();
    ~GhostWire();

    /// Загрузить библиотеку из пути или из директории приложения
    bool load(const QString& libDir = QString());

    /// Создать экземпляр прокси из конфигурационной строки
    bool create(const QString& configJson);

    /// Уничтожить экземпляр
    void destroy();

    /// Запустить / остановить
    bool start();
    void stop();

    bool isRunning() const;
    GhostWireProxyState state() const;

    /// Получить текущую статистику
    GhostWireProxyStats getStats() const;

    /// Загружена ли библиотека
    bool isLoaded() const;

private:
    QLibrary m_lib;
    GwProxy* m_handle = nullptr;

    using FnCreateFromFile = GwProxy*(*)(const char*);
    using FnCreate         = GwProxy*(*)(const char*);
    using FnFree           = void(*)(GwProxy*);
    using FnStart          = int(*)(GwProxy*);
    using FnStop           = void(*)(GwProxy*);
    using FnGetState       = GhostWireProxyState(*)(const GwProxy*);
    using FnGetStats       = void(*)(const GwProxy*, GhostWireProxyStats*);

    FnCreateFromFile m_createFromFile = nullptr;
    FnCreate         m_create         = nullptr;
    FnFree           m_free           = nullptr;
    FnStart          m_start          = nullptr;
    FnStop           m_stop           = nullptr;
    FnGetState       m_getState       = nullptr;
    FnGetStats       m_getStats       = nullptr;

    void resolveSymbols();
};
