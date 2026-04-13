#include "TrayManager.h"
#include "Config.h"
#include <QApplication>
#include <QPixmap>
#include <QDebug>
#include <QCursor>

TrayManager::TrayManager(QObject* parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_animTimer(new QTimer(this))
{
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &TrayManager::onTrayActivated);
    connect(m_animTimer, &QTimer::timeout,
            this, &TrayManager::onAnimTick);
}

TrayManager::~TrayManager() = default;

void TrayManager::loadIcons() {
    // Загружаем статические иконки из ресурсов
    m_idleIcon = QIcon(Config::TRAY_ICON_IDLE);
    m_activeIcon = QIcon(Config::TRAY_ICON_ACTIVE);

    if (m_idleIcon.isNull())
        qWarning() << "TrayManager: не загружена иконка покоя" << Config::TRAY_ICON_IDLE;
    if (m_activeIcon.isNull())
        qWarning() << "TrayManager: не загружена иконка активности" << Config::TRAY_ICON_ACTIVE;

    // Загружаем кадры покадровой анимации
    m_animFrames.clear();
    for (int i = 1; i <= Config::TRAY_ANIM_FRAME_COUNT; ++i) {
        QString path = QString::fromUtf8(Config::TRAY_ANIM_FRAME_PATTERN)
            .arg(i, 2, 10, QChar('0'));
        QIcon frame(path);
        if (frame.isNull()) {
            qWarning() << "TrayManager: не загружен кадр анимации" << path;
        }
        m_animFrames.append(frame);
    }

    if (!m_animFrames.isEmpty())
        qDebug() << "TrayManager: загружено кадров анимации:" << m_animFrames.size();
}

void TrayManager::init() {
    loadIcons();

    // Иконка покоя — tooltip с версией
    m_trayIcon->setIcon(m_idleIcon);
    QString version = QCoreApplication::applicationVersion();
    m_trayIcon->setToolTip(
        version.isEmpty()
            ? QStringLiteral("GhostWire Desktop")
            : QStringLiteral("GhostWire Desktop v%1").arg(version)
    );
    m_trayIcon->setVisible(true);

    // Таймер покадровой анимации (запускается только при активном режиме)
    m_animTimer->setInterval(Config::TRAY_ANIM_INTERVAL_MS);
}

void TrayManager::cleanup() {
    m_trayIcon->setVisible(false);
    m_animTimer->stop();
    m_animFrames.clear();
}

void TrayManager::setState(bool running) {
    m_running = running;
    m_animFrameIndex = 0;

    if (running) {
        // Прокси запущен — показываем статическую ACTIVE иконку.
        // Анимация включится отдельно через setConnectionsState(), когда появятся WS-соединения.
        m_trayIcon->setIcon(m_activeIcon);
        m_animTimer->stop();
    } else {
        // Прокси остановлен — показываем IDLE иконку, сбрасываем состояние соединений
        m_hasConnections = false;
        m_trayIcon->setIcon(m_idleIcon);
        if (m_animTimer->isActive())
            m_animTimer->stop();
    }
}

void TrayManager::setConnectionsState(bool hasConnections) {
    m_hasConnections = hasConnections;

    if (!m_running) {
        // Если прокси не запущен — игнорируем, остаёмся на IDLE
        return;
    }

    if (hasConnections) {
        // Есть WS-соединения — запускаем анимацию
        if (!m_animFrames.isEmpty()) {
            m_trayIcon->setIcon(m_animFrames[0]);
            m_animFrameIndex = 0;
            if (!m_animTimer->isActive())
                m_animTimer->start();
        } else {
            // Нет кадров — fallback на ACTIVE
            m_trayIcon->setIcon(m_activeIcon);
        }
    } else {
        // Нет соединений — возвращаемся к статической ACTIVE иконке
        m_trayIcon->setIcon(m_activeIcon);
        if (m_animTimer->isActive())
            m_animTimer->stop();
    }
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    // Левый и правый клик открывают меню
    if (reason == QSystemTrayIcon::Context || reason == QSystemTrayIcon::Trigger) {
        qDebug() << "TrayManager: tray activated, reason =" 
                 << (reason == QSystemTrayIcon::Context ? "Context" : "Trigger");
        emit iconClicked(QRect(QCursor::pos(), QSize(1, 1)));
    }
}

void TrayManager::onAnimTick() {
    if (!m_running || !m_hasConnections) return;
    if (m_animFrames.isEmpty()) return;

    int nextFrame = (m_animFrameIndex + 1) % m_animFrames.size();
    if (nextFrame != m_animFrameIndex) {
        m_animFrameIndex = nextFrame;
        m_trayIcon->setIcon(m_animFrames[m_animFrameIndex]);
    }
}

void TrayManager::showMessage(const QString& title, const QString& message,
                              QSystemTrayIcon::MessageIcon icon,
                              int millisecondsTimeout) {
    m_trayIcon->showMessage(title, message, icon, millisecondsTimeout);
}
