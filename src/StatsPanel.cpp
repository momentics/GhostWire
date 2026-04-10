#include "StatsPanel.h"
#include <QFont>
#include <QApplication>
#include <QHBoxLayout>

StatsPanel::StatsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 2, 6, 2);
    layout->setSpacing(2);

    QFont font = this->font();
    font.setPointSize(8);

    // Верхняя строка: "Работает" слева, версия справа
    auto* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    m_labelUptime = new QLabel(this);
    m_labelUptime->setFont(font);
    topLayout->addWidget(m_labelUptime);

    topLayout->addStretch();

    m_labelVersion = new QLabel(this);
    m_labelVersion->setFont(font);
    m_labelVersion->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_labelVersion->setStyleSheet("color: #888;");
    QString version = QCoreApplication::applicationVersion();
    if (!version.isEmpty()) {
        m_labelVersion->setText(QString("v%1").arg(version));
    }
    topLayout->addWidget(m_labelVersion);
    layout->addLayout(topLayout);

    m_labelWebSocket = new QLabel(this);
    m_labelWebSocket->setFont(font);
    layout->addWidget(m_labelWebSocket);

    m_labelPeak = new QLabel(this);
    m_labelPeak->setFont(font);
    m_labelPeak->setStyleSheet("margin-top: 4px;");
    layout->addWidget(m_labelPeak);

    m_labelTotal = new QLabel(this);
    m_labelTotal->setFont(font);
    m_labelTotal->setStyleSheet("margin-top: 2px;");
    layout->addWidget(m_labelTotal);

    layout->addStretch();
}

void StatsPanel::updateStats(uint64_t uptimeSecs, uint64_t websocketActive,
                             double peakRx, double peakTx,
                             uint64_t totalRx, uint64_t totalTx) {
    if (uptimeSecs == 0 && websocketActive == 0) {
        m_labelUptime->setText("");
        m_labelWebSocket->clear();
        m_labelPeak->clear();
        m_labelTotal->clear();
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

    // Пик скорости
    m_labelPeak->setText(
        QString("Пик:              ↑%1  ↓%2")
            .arg(formatBytes(peakRx))
            .arg(formatBytes(peakTx))
    );

    // Всего трафика
    m_labelTotal->setText(
        QString("Всего:           ↑%1  ↓%2")
            .arg(formatBytes(totalRx))
            .arg(formatBytes(totalTx))
    );
}

QString StatsPanel::formatBytes(double bytes) {
    if (bytes < 1024.0)
        return QString("%1 Б").arg(static_cast<int>(bytes));
    if (bytes < 1024.0 * 1024.0)
        return QString("%1 КБ").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024.0 * 1024.0 * 1024.0)
        return QString("%1 МБ").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    if (bytes < 1024.0 * 1024.0 * 1024.0 * 1024.0)
        return QString("%1 ГБ").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    return QString("%1 ТБ").arg(bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}
