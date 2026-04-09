#pragma once

#include <QWidget>
#include <QAction>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>

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
    void setStats(uint64_t uptimeSecs, uint64_t websocketActive);

    /// Добавить точку данных в спарклайн (дельта RX/TX в байтах)
    void addSparklinePoint(double rxBytesPerSec, double txBytesPerSec);

    /// Очистить спарклайн
    void clearSparkline();

    /// Обновить текст кнопки Старт/Стоп
    void setRunningState(bool running);

signals:
    void startRequested();
    void stopRequested();
    void exitRequested();

protected:
    bool event(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    StatsPanel*       m_statsPanel     = nullptr;
    SparklineWidget*  m_sparkline      = nullptr;
    QPushButton*      m_toggleButton   = nullptr;
    QPushButton*      m_exitButton     = nullptr;

    void buildLayout();
};
