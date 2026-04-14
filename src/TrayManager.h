#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QIcon>
#include <QVector>
#include <QRect>

/// Управляет иконкой в системном трее:
/// - регистрация / удаление иконки
/// - покадровая анимация при активном режиме
/// - передача геометрии иконки для позиционирования меню
class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(QObject* parent = nullptr);
    ~TrayManager();

    /// Инициализировать иконку в трее (загружает иконки из ресурсов)
    void init();

    /// Очистить иконку из трея
    void cleanup();

    /// Установить состояние: остановлен / запущен (переключает между IDLE и ACTIVE)
    void setState(bool running);

    /// Установить состояние соединений: есть WS-соединения / нет (переключает между ACTIVE и анимацией)
    void setConnectionsState(bool hasConnections);

    /// Показать временное всплывающее сообщение (toast) от иконки трея
    void showMessage(const QString& title, const QString& message,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                     int millisecondsTimeout = 3000);

    /// Получить геометрию иконки трея в экранных координатах.
    /// На Windows обычно работает корректно. На Linux/macOS может возвращать
    /// пустой QRect — в этом случае вызывающая сторона использует курсор.
    QRect trayIconGeometry() const;

signals:
    /// Пользователь кликнул по иконке — открыть меню
    void iconClicked(const QRect& iconGeometry);

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onAnimTick();

private:
    QSystemTrayIcon* m_trayIcon = nullptr;
    QTimer*          m_animTimer = nullptr;
    bool             m_running = false;
    bool             m_hasConnections = false;
    int              m_animFrameIndex = 0;

    /// Загруженные кадры анимации
    QVector<QIcon> m_animFrames;

    /// Кэшированные иконки
    QIcon m_idleIcon;
    QIcon m_activeIcon;

#ifdef Q_OS_LINUX
    QWidget* m_fallbackDock = nullptr; ///< Fallback dock при отсутствии системного трея
#endif

    void loadIcons();
};
