#include "StatsPanel.h"
#include "Utils.h"
#include <QFont>
#include <QApplication>
#include <QHBoxLayout>
#include <QFontMetrics>

StatsPanel::StatsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 2, 6, 2);
    mainLayout->setSpacing(2);

    QFont font = this->font();
    font.setPointSize(8);

    // Грид: 2 колонки — 4 строки (Работает, Соединений, Пик, Всего)
    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setVerticalSpacing(2);
    m_gridLayout->setHorizontalSpacing(0);

    auto* labelUptimeText = new QLabel(tr("Работает:"), this);
    labelUptimeText->setFont(font);
    labelUptimeText->setStyleSheet("color: #aaa;");
    m_gridLayout->addWidget(labelUptimeText, 0, 0, Qt::AlignLeft);

    m_labelUptime = new QLabel(this);
    m_labelUptime->setFont(font);
    m_gridLayout->addWidget(m_labelUptime, 0, 1, Qt::AlignLeft);

    auto* labelConnections = new QLabel(tr("Соединений:"), this);
    labelConnections->setFont(font);
    labelConnections->setStyleSheet("color: #aaa;");
    m_gridLayout->addWidget(labelConnections, 1, 0, Qt::AlignLeft);

    m_labelConnections = new QLabel(this);
    m_labelConnections->setFont(font);
    m_gridLayout->addWidget(m_labelConnections, 1, 1, Qt::AlignLeft);

    auto* labelPeak = new QLabel(tr("Пик:"), this);
    labelPeak->setFont(font);
    labelPeak->setStyleSheet("color: #aaa;");
    m_gridLayout->addWidget(labelPeak, 2, 0, Qt::AlignLeft);

    m_labelPeak = new QLabel(this);
    m_labelPeak->setFont(font);
    m_gridLayout->addWidget(m_labelPeak, 2, 1, Qt::AlignLeft);

    auto* labelTotal = new QLabel(tr("Всего:"), this);
    labelTotal->setFont(font);
    labelTotal->setStyleSheet("color: #aaa;");
    m_gridLayout->addWidget(labelTotal, 3, 0, Qt::AlignLeft);

    m_labelTotal = new QLabel(this);
    m_labelTotal->setFont(font);
    m_gridLayout->addWidget(m_labelTotal, 3, 1, Qt::AlignLeft);

    // Колонка 1 (значения) растягивается до конца
    m_gridLayout->setColumnStretch(1, 1);

    // Ширина колонки 0 = самый длинный лейбл
    QFontMetrics fm(font);
    int maxLabelWidth = 0;
    for (const auto* lbl : {labelUptimeText, labelConnections, labelPeak, labelTotal}) {
        int w = fm.horizontalAdvance(lbl->text());
        if (w > maxLabelWidth) maxLabelWidth = w;
    }
    m_gridLayout->setColumnMinimumWidth(0, maxLabelWidth);

    mainLayout->addLayout(m_gridLayout);

    // Версия — поверх грида, справа, без добавления высоты
    m_labelVersion = new QLabel(this);
    m_labelVersion->setFont(font);
    m_labelVersion->setStyleSheet("color: #888; background: transparent;");
    m_labelVersion->setAttribute(Qt::WA_TransparentForMouseEvents);
    QString version = QCoreApplication::applicationVersion();
    if (!version.isEmpty()) {
        m_labelVersion->setText(QString("v%1").arg(version));
    }
    m_labelVersion->show();
    m_labelVersion->raise();
    mainLayout->addStretch();
}

void StatsPanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // Позиционируем версию по центру первой строки грида
    if (m_labelVersion) {
        int x = width() - m_labelVersion->sizeHint().width();
        // Центр первой строки грида
        int y = m_gridLayout->itemAtPosition(0, 0)->widget()->y() +
                m_gridLayout->itemAtPosition(0, 0)->widget()->height() / 2 -
                m_labelVersion->height() / 2;
        m_labelVersion->move(x, y);
    }
}

void StatsPanel::updateStats(uint64_t uptimeSecs, uint64_t websocketActive,
                             double peakRx, double peakTx,
                             uint64_t totalRx, uint64_t totalTx) {
    if (uptimeSecs == 0 && websocketActive == 0) {
        m_labelUptime->setText("");
        m_labelConnections->clear();
        m_labelPeak->clear();
        m_labelTotal->clear();
        return;
    }

    uint64_t totalSecs = uptimeSecs;
    uint64_t days = totalSecs / 86400;
    uint64_t h = (totalSecs % 86400) / 3600;
    uint64_t m = (totalSecs % 3600) / 60;
    uint64_t s = totalSecs % 60;

    if (days > 0) {
        m_labelUptime->setText(
            QString("%1, %2:%3:%4")
                .arg(days, 3, 10, QLatin1Char('0'))
                .arg(h, 2, 10, QLatin1Char('0'))
                .arg(m, 2, 10, QLatin1Char('0'))
                .arg(s, 2, 10, QLatin1Char('0'))
        );
    } else {
        m_labelUptime->setText(
            QString("%1:%2:%3")
                .arg(h, 2, 10, QLatin1Char('0'))
                .arg(m, 2, 10, QLatin1Char('0'))
                .arg(s, 2, 10, QLatin1Char('0'))
        );
    }

    m_labelConnections->setText(QString::number(websocketActive));

    m_labelPeak->setText(
        QString("↓%1 ↑%2")
            .arg(formatBytes(peakRx))
            .arg(formatBytes(peakTx))
    );

    m_labelTotal->setText(
        QString("↓%1 ↑%2")
            .arg(formatBytes(totalRx))
            .arg(formatBytes(totalTx))
    );
}
