#pragma once

#include <QString>
#include <QLibrary>
#include <memory>
#include "IGhostWire.h"

class GhostWire : public IGhostWire {
public:
    GhostWire();
    ~GhostWire() override;

    bool load(const QString& libDir = QString()) override;
    bool create(const QString& configJson) override;
    void destroy() override;
    bool start() override;
    void stop() override;
    GhostWireProxyState state() const override;
    GhostWireProxyStats getStats() const override;

    bool isRunning() const;
    bool isLoaded() const;

private:
    QLibrary m_lib;
    GwProxy* m_handle = nullptr;

    using FnCreate = GwProxy*(*)(const char*);
    using FnFree   = void(*)(GwProxy*);
    using FnStart          = int(*)(GwProxy*);
    using FnStop           = void(*)(GwProxy*);
    using FnGetState       = GhostWireProxyState(*)(const GwProxy*);
    using FnGetStats       = void(*)(const GwProxy*, GhostWireProxyStats*);

    FnCreate m_create = nullptr;
    FnFree   m_free   = nullptr;
    FnStart          m_start          = nullptr;
    FnStop           m_stop           = nullptr;
    FnGetState       m_getState       = nullptr;
    FnGetStats       m_getStats       = nullptr;

    void resolveSymbols();
};
