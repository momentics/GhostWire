#pragma once

#include <QObject>
#include <QTimer>
#include <QRect>
#include <QFile>
#include <memory>

class TrayManager;
class TrayMenu;
class GhostWire;

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

private slots:
    void onStatsTick();
    void onTrayExit();
    void onConfigureTelegram();

private:
    std::unique_ptr<GhostWire>   m_ghostWire;
    std::unique_ptr<TrayManager> m_trayManager;
    std::unique_ptr<TrayMenu>    m_trayMenu;
    QTimer*                      m_statsTimer = nullptr;

    // Для расчёта дельты RX/TX (храним только нужные поля)
    uint64_t m_prevBytesReceived = 0;
    uint64_t m_prevBytesSent = 0;
    double   m_peakRx = 0;
    double   m_peakTx = 0;
    bool     m_hasPrevStats = false;

    // Явное состояние прокси: true = запущен, false = остановлен
    bool     m_proxyRunning = false;

    /// Загрузить конфиг из ресурсов
    QString loadConfig() const;

    /// Восстановить предыдущее состояние (Start/Stop) из QSettings
    void restoreState();

    /// Сохранить текущее состояние в QSettings
    void saveState();

    /// Обработать обновление статистики
    void processStats(const GhostWireProxyStats& stats);

    /// Показать/скрыть контекстное меню
    void showTrayMenu(const QRect& iconRect);
    void hideTrayMenu();
};
