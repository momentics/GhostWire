#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QRect>
#include "../libs/ghostwire/include/ghostwire.h"

/// Абстракция управления иконкой трея: инициализация, состояние, уведомления.
class ITrayManager : public QObject {
    Q_OBJECT
public:
    explicit ITrayManager(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ITrayManager() = default;

    virtual void init() = 0;
    virtual void cleanup() = 0;
    virtual void setState(GhostWireProxyState state) = 0;
    virtual void setConnectionsState(bool hasConnections) = 0;
    virtual void showMessage(const QString& title, const QString& message,
                             QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                             int millisecondsTimeout = 3000) = 0;
    virtual QRect trayIconGeometry() const = 0;
    virtual QSystemTrayIcon* trayIcon() const = 0;

signals:
    void iconClicked(const QRect& iconGeometry);

#ifdef Q_OS_LINUX
    void linuxQuitRequested();
#endif
};
