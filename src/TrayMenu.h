#pragma once

#include <QWidget>
#include <QPushButton>
#include <QTimer>
#include <QEvent>
#include <QShowEvent>
#include <QHideEvent>

class StatsPanel;
class SparklineWidget;

/// Borderless popup-окно, имитирующее контекстное меню трея.
/// Содержит: статистику, график RX/TX, кнопки Старт/Стоп и Выход.
class TrayMenu : public QWidget {
    Q_OBJECT
public:
    explicit TrayMenu(QWidget* parent = nullptr);
    ~TrayMenu();

    /// Обновить статистику (вызывается из Application по таймеру)
    void setStats(uint64_t uptimeSecs, uint64_t websocketActive,
                  double peakRx, double peakTx,
                  uint64_t totalRx, uint64_t totalTx);

    /// Добавить точку данных в спарклайн (дельта RX/TX в байтах)
    void addSparklinePoint(double rxBytesPerSec, double txBytesPerSec);

    /// Очистить спарклайн
    void clearSparkline();

    /// Обновить текст кнопки Старт/Стоп
    void setRunningState(bool running);

    /// Скрыть меню
    void hideMenu();

    /// Начать проверку потери фокуса (для IPC-показа).
    /// Вызывается один раз после показа меню из второго экземпляра.
    /// При обычном показе мышью НЕ вызывается — там работают FocusOut/WindowDeactivate.
    void startIpcFocusMonitor();

signals:
    void startRequested();
    void stopRequested();
    void exitRequested();
    void configureTelegramRequested();
    void checkUpdatesRequested();

protected:
    bool event(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    StatsPanel*       m_statsPanel     = nullptr;
    SparklineWidget*  m_sparkline      = nullptr;
    QPushButton*      m_toggleButton   = nullptr;
    QPushButton*      m_telegramButton = nullptr;
    QPushButton*      m_updateButton   = nullptr;
    QPushButton*      m_exitButton     = nullptr;

    void buildLayout();
    void tryHideMenu();
    static Qt::WindowFlags makeWindowFlags();

    bool m_isRunning = false;
    QTimer* m_autoHideTimer = nullptr;
    bool    m_ipcMode = false; ///< true — показ через IPC, false — показ кликом по иконке
    bool    m_wasUnderMouse = false; ///< при IPC-показе: была ли мышь над меню хотя бы раз
};
