#pragma once

#include <QObject>
#include <QTimer>
#include <QRect>
#include <memory>

class TrayManager;
class TrayMenu;
class GhostWire;
class UpdateChecker;

// Forward — определено в GhostWire.h
struct GhostWireProxyStats;

/// Главный класс приложения.
/// Связывает TrayManager, GhostWire и TrayMenu.
class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject* parent = nullptr);
    ~Application();

    /// Инициализация: загрузка библиотеки, конфиг, трей
    bool initialize();

    /// Показать контекстное меню у текущей позиции курсора.
    /// Вызывается при повторном запуске приложения (второй экземпляр просит показать меню).
    void showTrayMenuAtCursor();

private slots:
    void onStatsTick();
    void onTrayExit();
    void onConfigureTelegram();
    void onCheckUpdatesRequested();
    void onUpdateAvailable(const QString& version, const QString& releaseUrl);
    void onNoUpdate();
    void onUpdateCheckFailed(const QString& error);

private:
    std::unique_ptr<GhostWire>   m_ghostWire;
    std::unique_ptr<TrayManager> m_trayManager;
    std::unique_ptr<TrayMenu>    m_trayMenu;
    std::unique_ptr<QTimer>      m_statsTimer;
    std::unique_ptr<UpdateChecker> m_updateChecker;

    // Для расчёта дельты RX/TX (храним только нужные поля)
    uint64_t m_prevBytesReceived = 0;
    uint64_t m_prevBytesSent = 0;
    double   m_peakRx = 0;
    double   m_peakTx = 0;
    bool     m_hasPrevStats = false;

    // Для отслеживания изменений количества WS-соединений
    uint64_t m_prevWsActive = 0;

    // Явное состояние прокси: true = запущен, false = остановлен
    bool     m_proxyRunning = false;

    // Флаг ручной проверки обновлений
    bool     m_isManualUpdateCheck = false;

    /// Загрузить конфиг из ресурсов
    QString loadConfig() const;

    /// Восстановить предыдущее состояние (Start/Stop) из QSettings
    void restoreState();

    /// Сохранить текущее состояние в QSettings
    void saveState();

    /// Показать контекстное меню
    void showTrayMenu(const QRect& iconRect);

    /// Показать контекстное меню в заданной экранных точке.
    /// Содержит общую логику: screen correction, move, raise, show, activateWindow.
    void showTrayMenuAtPoint(const QPoint& pos);

    /// Проверить, зарегистрирован ли обработчик tg:// протокола
    bool isTelegramSchemeRegistered() const;
};
