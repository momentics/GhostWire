#pragma once

#include <QObject>
#include <QTimer>
#include <QRect>
#include <memory>

class TrayMenu;
class UpdateChecker;
class UpdateNotifier;
class StatsTracker;
class SettingsManager;
class IGhostWire;
class ITrayManager;

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
    void onUpdateAvailable(const QString& version, const QString& releaseUrl, bool manual);
    void onNoUpdate(bool manual);
    void onUpdateCheckFailed(const QString& error, bool manual);
    void onOpenReleaseUrl(const QString& url);

private:
    std::unique_ptr<IGhostWire>   m_ghostWire;
    std::unique_ptr<ITrayManager> m_trayManager;
    std::unique_ptr<TrayMenu>     m_trayMenu;
    std::unique_ptr<QTimer>      m_statsTimer;
    std::unique_ptr<QTimer>      m_updateCheckTimer;
    std::unique_ptr<UpdateChecker> m_updateChecker;
    std::unique_ptr<UpdateNotifier> m_updateNotifier;
    std::unique_ptr<StatsTracker> m_statsTracker;
    std::unique_ptr<SettingsManager> m_settings;

    // Явное состояние прокси: true = запущен, false = остановлен
    bool     m_proxyRunning = false;

    /// Загрузить конфиг из ресурсов
    QString loadConfig() const;

    /// Восстановить предыдущее состояние (Start/Stop) из QSettings
    void restoreState();

    /// Запустить прокси и синхронизировать UI. Возвращает false и показывает уведомление при отказе запуска.
    bool startProxy();

    /// Сохранить текущее состояние в QSettings
    void saveState();

    /// Запланировать следующую автоматическую проверку обновлений.
    void scheduleNextUpdateCheck();

    /// Показать контекстное меню
    void showTrayMenu(const QRect& iconRect);

    /// Показать контекстное меню в заданной экранных точке.
    /// Содержит общую логику: screen correction, move, raise, show, activateWindow.
    void showTrayMenuAtPoint(const QPoint& pos);

    /// Проверить, зарегистрирован ли обработчик tg:// протокола
    bool isTelegramSchemeRegistered() const;
};
