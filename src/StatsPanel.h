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
    void updateStats(uint64_t uptimeSecs, uint64_t websocketActive);

private:
    QLabel* m_labelUptime;
    QLabel* m_labelWebSocket;
};
