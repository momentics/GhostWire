#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QVector>

class QEvent;

/// Панель числовой статистики с выравниванием через QGridLayout
class StatsPanel : public QWidget {
    Q_OBJECT
public:
    explicit StatsPanel(QWidget* parent = nullptr);

    /// Обновить значения
    void updateStats(uint64_t uptimeSecs,
                     uint64_t websocketActive, uint64_t websocketPeak,
                     uint64_t ipRotations, uint64_t ipSucRotations,
                     double peakRx, double peakTx,
                     uint64_t totalRx, uint64_t totalTx);

protected:
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyPlatformStyle();

    QLabel* m_labelUptime = nullptr;
    QLabel* m_labelVersion = nullptr;
    QLabel* m_labelConnections = nullptr;
    QLabel* m_labelRotations = nullptr;
    QLabel* m_labelPeak = nullptr;
    QLabel* m_labelTotal = nullptr;
    QVector<QLabel*> m_secondaryLabels;

    QGridLayout* m_gridLayout = nullptr;
};
