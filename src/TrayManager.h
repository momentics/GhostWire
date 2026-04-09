#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QIcon>
#include <QVector>
#include <QRect>

class TrayMenu;

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

    /// Установить состояние: остановлен / активен (переключает анимацию)
    void setState(bool running);

    /// Получить глобальный прямоугольник иконки в трее (для позиционирования меню)
    QRect getGeometry() const;

    /// Показать временное всплывающее сообщение (toast) от иконки трея
    void showMessage(const QString& title, const QString& message,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                     int millisecondsTimeout = 3000);

signals:
    /// Пользователь кликнул по иконке — открыть меню
    void iconClicked(const QRect& iconGeometry);

    /// Пользователь запросил выход
    void exitRequested();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onAnimTick();

private:
    QSystemTrayIcon* m_trayIcon = nullptr;
    QTimer*          m_animTimer = nullptr;
    bool             m_running = false;
    int              m_animFrameIndex = 0;

    /// Загруженные кадры анимации
    QVector<QIcon> m_animFrames;

    /// Кэшированные иконки
    QIcon m_idleIcon;
    QIcon m_activeIcon;

    void loadIcons();
};
