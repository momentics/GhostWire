#pragma once

#include <QIcon>
#include <QRect>

#include "ITrayManager.h"

class MacTrayManager : public ITrayManager {
    Q_OBJECT
public:
    explicit MacTrayManager(QObject* parent = nullptr);
    ~MacTrayManager() override;

    void init() override;
    void cleanup() override;
    void setState(GhostWireProxyState state) override;
    void setConnectionsState(bool hasConnections) override;
    void showMessage(const QString& title, const QString& message,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                     int millisecondsTimeout = 3000) override;
    QRect trayIconGeometry() const override;
    QSystemTrayIcon* trayIcon() const override { return nullptr; }

    void handleStatusItemActivated();

private:
    void* m_statusItem = nullptr;
    void* m_target = nullptr;
    GhostWireProxyState m_state = GHOSTWIRE_PROXY_OFFLINE;
    bool m_hasConnections = false;

    void updateStatusImage();
};
