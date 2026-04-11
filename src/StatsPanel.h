#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>

/// Панель числовой статистики с выравниванием через QGridLayout
class StatsPanel : public QWidget {
    Q_OBJECT
public:
    explicit StatsPanel(QWidget* parent = nullptr);

    /// Обновить значения
    void updateStats(uint64_t uptimeSecs, uint64_t websocketActive,
                     double peakRx, double peakTx,
                     uint64_t totalRx, uint64_t totalTx);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* m_labelUptime;
    QLabel* m_labelVersion;
    QLabel* m_labelConnections;
    QLabel* m_labelPeak;
    QLabel* m_labelTotal;

    QGridLayout* m_gridLayout;
};
