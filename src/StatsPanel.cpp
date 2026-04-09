#include "StatsPanel.h"
#include <QFont>

StatsPanel::StatsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 2, 6, 2);
    layout->setSpacing(2);

    QFont font = this->font();
    font.setPointSize(8);

    m_labelUptime = new QLabel(this);
    m_labelUptime->setFont(font);
    layout->addWidget(m_labelUptime);

    m_labelWebSocket = new QLabel(this);
    m_labelWebSocket->setFont(font);
    layout->addWidget(m_labelWebSocket);

    layout->addStretch();
}

void StatsPanel::updateStats(uint64_t uptimeSecs, uint64_t websocketActive) {
    if (uptimeSecs == 0 && websocketActive == 0) {
        m_labelUptime->setText("нет данных");
        m_labelWebSocket->clear();
        return;
    }

    // Форматируем uptime: HH:MM:SS
    uint64_t h = uptimeSecs / 3600;
    uint64_t m = (uptimeSecs % 3600) / 60;
    uint64_t s = uptimeSecs % 60;
    m_labelUptime->setText(
        QString("Работает:       %1:%2:%3")
            .arg(h, 2, 10, QLatin1Char('0'))
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'))
    );

    m_labelWebSocket->setText(
        QString("Соединений:  %1").arg(websocketActive)
    );
}
