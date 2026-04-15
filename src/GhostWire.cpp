#include "GhostWire.h"
#include "Config.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonParseError>

#ifdef _WIN32
#  include <windows.h>
#endif

// Примечание: структура GhostWireProxyStats должна совпадать по layout с
// GhostWireProxyStats из ghostwire.h. Мы определяем её идентично.

GhostWire::GhostWire() = default;

GhostWire::~GhostWire() {
    destroy();
    if (m_lib.isLoaded())
        m_lib.unload();
}

bool GhostWire::load(const QString& libDir) {
    if (m_lib.isLoaded())
        return true;

    QString libPath;

    if (!libDir.isEmpty()) {
#ifdef _WIN32
        libPath = QDir(libDir).filePath("ghostwire.dll");
#elif defined(__APPLE__)
        libPath = QDir(libDir).filePath("libghostwire.dylib");
#else
        libPath = QDir(libDir).filePath("libghostwire.so");
#endif
    }

    if (libPath.isEmpty() || !QFile::exists(libPath)) {
        // Fallback: из директории приложения
        QString appDir = QCoreApplication::applicationDirPath();
#ifdef _WIN32
        libPath = QDir(appDir).filePath("ghostwire.dll");
#elif defined(__APPLE__)
        libPath = QDir(appDir).filePath("libghostwire.dylib");
#else
        libPath = QDir(appDir).filePath("libghostwire.so");
#endif
    }

    m_lib.setFileName(libPath);

    if (!m_lib.load()) {
        qWarning() << "GhostWire: не удалось загрузить библиотеку:" << libPath
                   << m_lib.errorString();
        return false;
    }

    resolveSymbols();

    if (!m_createFromFile || !m_free || !m_start || !m_stop || !m_isRunning || !m_getStats) {
        qWarning() << "GhostWire: не все символы разрешены";
        m_lib.unload();
        return false;
    }

    qDebug() << "GhostWire: библиотека загружена:" << libPath;
    return true;
}

void GhostWire::resolveSymbols() {
    m_createFromFile = reinterpret_cast<FnCreateFromFile>(
        m_lib.resolve("ghostwire_proxy_create_from_file"));
    m_create = reinterpret_cast<FnCreate>(
        m_lib.resolve("ghostwire_proxy_create"));
    m_free = reinterpret_cast<FnFree>(
        m_lib.resolve("ghostwire_proxy_free"));
    m_start = reinterpret_cast<FnStart>(
        m_lib.resolve("ghostwire_proxy_start"));
    m_stop = reinterpret_cast<FnStop>(
        m_lib.resolve("ghostwire_proxy_stop"));
    m_isRunning = reinterpret_cast<FnIsRunning>(
        m_lib.resolve("ghostwire_proxy_is_running"));
    m_getStats = reinterpret_cast<FnGetStats>(
        m_lib.resolve("ghostwire_proxy_get_stats"));
}

bool GhostWire::create(const QString& configJson) {
    if (!m_lib.isLoaded()) {
        qWarning() << "GhostWire::create — библиотека не загружена";
        return false;
    }

    // Валидация JSON перед передачей в C-библиотеку
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "GhostWire::create — невалидный JSON:" << parseError.errorString();
        return false;
    }

    if (m_handle) {
        destroy();
    }

    // держим configData. Это не лик
    QByteArray configData = configJson.toUtf8();

    // Пробуем create (из строки)
    if (m_create) {
        m_handle = m_create(configData.constData());
    }

    //if (!m_handle && m_createFromFile) {
    //    m_handle = m_createFromFile(configData.constData());
    // }

    if (!m_handle) {
        qWarning() << "GhostWire::create — не удалось создать экземпляр";
        return false;
    }

    qDebug() << "GhostWire::create — экземпляр создан";
    return true;
}

void GhostWire::destroy() {
    if (m_handle && m_free) {
        m_free(m_handle);
        m_handle = nullptr;
        qDebug() << "GhostWire::destroy — экземпляр уничтожен";
    }
}

bool GhostWire::start() {
    if (!m_handle || !m_start) return false;
    int ret = m_start(m_handle);
    qDebug() << "GhostWire::start — результат:" << ret;
    return ret == 0;
}

void GhostWire::stop() {
    if (!m_handle || !m_stop) return;
    m_stop(m_handle);
    qDebug() << "GhostWire::stop";
}

bool GhostWire::isRunning() const {
    if (!m_handle || !m_isRunning) return false;
    return m_isRunning(m_handle);
}

GhostWireProxyStats GhostWire::getStats() const {
    GhostWireProxyStats stats{};
    if (m_handle && m_getStats) {
        m_getStats(m_handle, &stats);
    }
    return stats;
}

bool GhostWire::isLoaded() const {
    return m_lib.isLoaded();
}
