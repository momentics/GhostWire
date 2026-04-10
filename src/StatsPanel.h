#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

/// Панель числовой статистики
class StatsPanel : public QWidget {
    Q_OBJECT
public:
    explicit StatsPanel(QWidget* parent = nullptr);

    /// Обновить значения
    void updateStats(uint64_t uptimeSecs, uint64_t websocketActive,
                     double peakRx, double peakTx,
                     uint64_t totalRx, uint64_t totalTx);

private:
    QLabel* m_labelUptime;
    QLabel* m_labelVersion;
    QLabel* m_labelWebSocket;
    QLabel* m_labelPeak;
    QLabel* m_labelTotal;

    static QString formatBytes(double bytes);
};
