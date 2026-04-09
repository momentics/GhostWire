#pragma once

#include <QString>
#include <QLibrary>
#include <cstdint>
#include <memory>

struct GhostWireProxyStats {
    bool     running;
    uint64_t active_connections;
    uint64_t total_connections;
    uint64_t websocket_active;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    uint64_t errors;
    uint64_t ip_rotations;
    uint64_t media_connections;
    uint64_t uptime_secs;
    uint64_t peak_active_connections;
    uint64_t rotation_success;
};

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

    /// Получить текущую статистику
    GhostWireProxyStats getStats() const;

    /// Загружена ли библиотека
    bool isLoaded() const;

private:
    QLibrary m_lib;
    void*    m_handle = nullptr;

    using FnCreateFromFile = void*(*)(const char*);
    using FnCreate         = void*(*)(const char*);
    using FnFree           = void(*)(void*);
    using FnStart          = int(*)(void*);
    using FnStop           = void(*)(void*);
    using FnIsRunning      = bool(*)(const void*);
    using FnGetStats       = void(*)(const void*, void*);

    FnCreateFromFile m_createFromFile = nullptr;
    FnCreate         m_create         = nullptr;
    FnFree           m_free           = nullptr;
    FnStart          m_start          = nullptr;
    FnStop           m_stop           = nullptr;
    FnIsRunning      m_isRunning      = nullptr;
    FnGetStats       m_getStats       = nullptr;

    void resolveSymbols();
};
