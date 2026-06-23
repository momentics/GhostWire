#pragma once

#include <QString>
#include "../libs/ghostwire/include/ghostwire.h"

/// Абстракция прокси-библиотеки: загрузка, создание, управление жизненным циклом, статистика.
class IGhostWire {
public:
    virtual ~IGhostWire() = default;

    virtual bool load(const QString& libDir = QString()) = 0;
    virtual bool create(const QString& configJson) = 0;
    virtual void destroy() = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual GhostWireProxyState state() const = 0;
    virtual GhostWireProxyStats getStats() const = 0;
};
