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

    // Иконка покоя
    m_trayIcon->setIcon(m_idleIcon);
    m_trayIcon->setToolTip("GhostWire Desktop");
    m_trayIcon->setVisible(true);

    // Таймер покадровой анимации
    m_animTimer->setInterval(200); // 5 FPS
    m_animTimer->start();
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
        if (!m_animFrames.isEmpty()) {
            // Начинаем покадровую анимацию
            m_trayIcon->setIcon(m_animFrames[0]);
        } else {
            m_trayIcon->setIcon(m_activeIcon);
        }
    } else {
        m_trayIcon->setIcon(m_idleIcon);
    }
}

QRect TrayManager::getGeometry() const {
    return m_trayIcon->geometry();
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    // Левый и правый клик открывают меню
    if (reason == QSystemTrayIcon::Context || reason == QSystemTrayIcon::Trigger) {
        emit iconClicked(QRect(QCursor::pos(), QSize(1, 1)));
    }
}

void TrayManager::onAnimTick() {
    if (!m_running) return;
    if (m_animFrames.isEmpty()) return;

    m_animFrameIndex = (m_animFrameIndex + 1) % m_animFrames.size();
    m_trayIcon->setIcon(m_animFrames[m_animFrameIndex]);
}

void TrayManager::showMessage(const QString& title, const QString& message,
                              QSystemTrayIcon::MessageIcon icon,
                              int millisecondsTimeout) {
    m_trayIcon->showMessage(title, message, icon, millisecondsTimeout);
}
