#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QIcon>
#include <QVector>
#include <QRect>
#include <QEvent>
#include <memory>
#include "ITrayManager.h"

class TrayManager : public ITrayManager {
    Q_OBJECT
public:
    explicit TrayManager(QObject* parent = nullptr);
    ~TrayManager() override;

    void init() override;
    void cleanup() override;
    void setState(GhostWireProxyState state) override;
    void setConnectionsState(bool hasConnections) override;
    void showMessage(const QString& title, const QString& message,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                     int millisecondsTimeout = 3000) override;
    QRect trayIconGeometry() const override;
    QSystemTrayIcon* trayIcon() const override { return m_trayIcon.get(); }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onAnimTick();

private:
    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
    std::unique_ptr<QTimer>          m_animTimer;
    GhostWireProxyState m_state = GHOSTWIRE_PROXY_OFFLINE;
    bool             m_hasConnections = false;
    int              m_animFrameIndex = 0;

    /// Загруженные кадры анимации
    QVector<QIcon> m_animFrames;

    /// Кэшированные иконки
    QIcon m_idleIcon;
    QIcon m_activeIcon;
    QIcon m_degradedIcon;

#ifdef Q_OS_LINUX
    std::unique_ptr<QWidget> m_fallbackDock; ///< Fallback dock при отсутствии системного трея
#endif

    void loadIcons();
    void applyIconState();
    QString applyFallbackDockStyle() const;
};
