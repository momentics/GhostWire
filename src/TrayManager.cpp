#include "TrayManager.h"
#include "Config.h"
#include <QApplication>
#include <QPixmap>
#include <QDebug>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>

#ifdef Q_OS_LINUX
#include <QWidget>
#endif

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

#ifdef Q_OS_LINUX
    // Gnome Shell (Ubuntu 22.04+) не имеет системного трея.
    // Если AppIndicator не установлен — создаём fallback dock.
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        m_fallbackDock = new QWidget(nullptr,
            Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
        m_fallbackDock->setFixedSize(24, 24);
        m_fallbackDock->setAttribute(Qt::WA_ShowWithoutActivating);
        m_fallbackDock->setStyleSheet(
            "QWidget { background: transparent; border-radius: 4px; }"
            "QWidget:hover { background: rgba(255,255,255,30); }");

        // Позиционировать в нижний правый угол
        auto* screen = QGuiApplication::primaryScreen();
        if (screen) {
            auto geom = screen->availableGeometry();
            m_fallbackDock->move(geom.right() - 28, geom.bottom() - 28);
        }

        // При клике — показать меню
        connect(m_fallbackDock, &QWidget::mousePressEvent,
                this, [this](QMouseEvent* ev) {
            if (ev->button() == Qt::LeftButton || ev->button() == Qt::RightButton) {
                emit iconClicked(QRect());
            }
        });

        m_fallbackDock->show();
        qDebug() << "TrayManager: системный трей недоступен, fallback dock создан";

        // Иконку трея НЕ создаём — анимация не применяется
        m_animTimer->setInterval(Config::TRAY_ANIM_INTERVAL_MS);
        return;
    }
#endif

    // Обычный системный трей (Windows, или Linux с AppIndicator)
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
#ifdef Q_OS_LINUX
    if (m_fallbackDock) {
        m_fallbackDock->close();
        delete m_fallbackDock;
        m_fallbackDock = nullptr;
    }
#endif
    if (m_trayIcon) {
        m_trayIcon->setVisible(false);
    }
    m_animTimer->stop();
    m_animFrames.clear();
}

void TrayManager::setState(bool running) {
    m_running = running;
    m_animFrameIndex = 0;

#ifdef Q_OS_LINUX
    if (m_fallbackDock) {
        // Fallback dock: меняем иконку через stylesheet
        if (running) {
            m_fallbackDock->setStyleSheet(
                "QWidget { background: transparent; border: 2px solid #4CAF50; border-radius: 4px; }"
                "QWidget:hover { background: rgba(76,175,80,40); }");
        } else {
            m_fallbackDock->setStyleSheet(
                "QWidget { background: transparent; border: 2px solid #888; border-radius: 4px; }"
                "QWidget:hover { background: rgba(255,255,255,30); }");
        }
        m_fallbackDock->update();
        return;
    }
#endif

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

#ifdef Q_OS_LINUX
    if (m_fallbackDock) {
        // Fallback dock: мигающая граница при наличии соединений
        if (m_running && hasConnections) {
            m_fallbackDock->setStyleSheet(
                "QWidget { background: rgba(76,175,80,60); border: 2px solid #4CAF50; border-radius: 4px; }"
                "QWidget:hover { background: rgba(76,175,80,80); }");
        } else if (m_running) {
            m_fallbackDock->setStyleSheet(
                "QWidget { background: transparent; border: 2px solid #4CAF50; border-radius: 4px; }"
                "QWidget:hover { background: rgba(76,175,80,40); }");
        } else {
            m_fallbackDock->setStyleSheet(
                "QWidget { background: transparent; border: 2px solid #888; border-radius: 4px; }"
                "QWidget:hover { background: rgba(255,255,255,30); }");
        }
        m_fallbackDock->update();
        return;
    }
#endif

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

#ifdef Q_OS_LINUX
        // На Linux QSystemTrayIcon::geometry() всегда пустой.
        // Application определяет позицию через availableGeometry() (Вариант C).
        // Передаём пустой QRect как сигнал использовать panel-based позиционирование.
        emit iconClicked(QRect());
#else
        // Windows: передаём позицию курсора как геометрию иконки
        emit iconClicked(QRect(QCursor::pos(), QSize(1, 1)));
#endif
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

QRect TrayManager::trayIconGeometry() const {
    return m_trayIcon ? m_trayIcon->geometry() : QRect();
}
