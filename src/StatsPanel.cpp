#include "StatsPanel.h"
#include "Utils.h"
#include <QFont>
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QFontMetrics>

namespace {
bool isDarkPalette(const QPalette& palette) {
    return palette.color(QPalette::Window).lightness() < 128;
}

QString secondaryLabelStyle() {
#ifdef Q_OS_MAC
    return isDarkPalette(QApplication::palette())
        ? QStringLiteral("color: rgba(235, 235, 245, 153);")
        : QStringLiteral("color: rgba(60, 60, 67, 166);");
#else
    return QStringLiteral("color: #aaa;");
#endif
}

QString versionLabelStyle() {
#ifdef Q_OS_MAC
    return isDarkPalette(QApplication::palette())
        ? QStringLiteral("color: rgba(235, 235, 245, 112); background: transparent;")
        : QStringLiteral("color: rgba(60, 60, 67, 128); background: transparent;");
#else
    return QStringLiteral("color: #888; background: transparent;");
#endif
}
} // namespace

StatsPanel::StatsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 2, 6, 2);
    mainLayout->setSpacing(2);

    // Размер в пунктах сохраняет масштаб текста статистики согласованным с настройками ОС.
    QFont baseFont = QApplication::font();
    baseFont.setPointSizeF(8.25);

    // Грид: 2 колонки — 4 строки (Работает, Соединений, Пик, Всего)
    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setVerticalSpacing(2);
    m_gridLayout->setHorizontalSpacing(0);

    auto* labelUptimeText = new QLabel(tr("Работает:"), this);
    labelUptimeText->setFont(baseFont);
    m_gridLayout->addWidget(labelUptimeText, 0, 0, Qt::AlignLeft);
    m_secondaryLabels.append(labelUptimeText);

    m_labelUptime = new QLabel(this);
    m_labelUptime->setFont(baseFont);
    m_gridLayout->addWidget(m_labelUptime, 0, 1, Qt::AlignLeft);

    auto* labelConnections = new QLabel(tr("Соединений:"), this);
    labelConnections->setFont(baseFont);
    m_gridLayout->addWidget(labelConnections, 1, 0, Qt::AlignLeft);
    m_secondaryLabels.append(labelConnections);

    m_labelConnections = new QLabel(this);
    m_labelConnections->setFont(baseFont);
    m_gridLayout->addWidget(m_labelConnections, 1, 1, Qt::AlignLeft);

    auto* labelRotations = new QLabel(tr("Ротаций:"), this);
    labelRotations->setFont(baseFont);
    m_gridLayout->addWidget(labelRotations, 2, 0, Qt::AlignLeft);
    m_secondaryLabels.append(labelRotations);

    m_labelRotations = new QLabel(this);
    m_labelRotations->setFont(baseFont);
    m_gridLayout->addWidget(m_labelRotations, 2, 1, Qt::AlignLeft);

    auto* labelPeak = new QLabel(tr("Пик:"), this);
    labelPeak->setFont(baseFont);
    m_gridLayout->addWidget(labelPeak, 3, 0, Qt::AlignLeft);
    m_secondaryLabels.append(labelPeak);

    m_labelPeak = new QLabel(this);
    m_labelPeak->setFont(baseFont);
    m_gridLayout->addWidget(m_labelPeak, 3, 1, Qt::AlignLeft);

    auto* labelTotal = new QLabel(tr("Всего:"), this);
    labelTotal->setFont(baseFont);
    m_gridLayout->addWidget(labelTotal, 4, 0, Qt::AlignLeft);
    m_secondaryLabels.append(labelTotal);

    m_labelTotal = new QLabel(this);
    m_labelTotal->setFont(baseFont);
    m_gridLayout->addWidget(m_labelTotal, 4, 1, Qt::AlignLeft);

    // Колонка 1 (значения) растягивается до конца
    m_gridLayout->setColumnStretch(1, 1);

    // Ширина колонки 0 = самый длинный лейбл
    QFontMetrics fm(baseFont);
    int maxLabelWidth = 0;
    for (const auto* lbl : {labelUptimeText, labelConnections, labelRotations, labelPeak, labelTotal}) {
        int w = fm.horizontalAdvance(lbl->text());
        if (w > maxLabelWidth) maxLabelWidth = w;
    }
    m_gridLayout->setColumnMinimumWidth(0, maxLabelWidth);

    mainLayout->addLayout(m_gridLayout);

    // Версия — поверх грида, справа, без добавления высоты
    m_labelVersion = new QLabel(this);
    m_labelVersion->setFont(baseFont);
    m_labelVersion->setAttribute(Qt::WA_TransparentForMouseEvents);
    QString version = QCoreApplication::applicationVersion();
    if (!version.isEmpty()) {
        m_labelVersion->setText(QString("v%1").arg(version));
    }
    m_labelVersion->show();
    m_labelVersion->raise();
    mainLayout->addStretch();

    applyPlatformStyle();
}

bool StatsPanel::event(QEvent* event) {
    if (event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::PaletteChange) {
        applyPlatformStyle();
    }
    return QWidget::event(event);
}

void StatsPanel::applyPlatformStyle() {
    const QString secondaryStyle = secondaryLabelStyle();
    for (auto* label : m_secondaryLabels) {
        label->setStyleSheet(secondaryStyle);
    }

    if (m_labelVersion) {
        m_labelVersion->setStyleSheet(versionLabelStyle());
    }
}

void StatsPanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (!m_labelVersion || !m_gridLayout)
        return;

    QLayoutItem* item = m_gridLayout->itemAtPosition(0, 0);
    if (!item)
        return;

    QWidget* refWidget = item->widget();
    if (!refWidget)
        return;

    int x = width() - m_labelVersion->sizeHint().width();
    int y = refWidget->y() + refWidget->height() / 2 - m_labelVersion->height() / 2;
    m_labelVersion->move(x, y);
}

void StatsPanel::updateStats(uint64_t uptimeSecs, uint64_t websocketActive, uint64_t websocketPeak,
                             uint64_t ipRotations, uint64_t ipSucRotations, 
                             double peakRx, double peakTx,
                             uint64_t totalRx, uint64_t totalTx) {
    if (uptimeSecs == 0 && websocketActive == 0) {
        m_labelUptime->setText("");
        m_labelConnections->clear();
        m_labelRotations->clear();
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

    m_labelConnections->setText(
        QString("%1 (%2)")
            .arg(QString::number(websocketActive))
            .arg(QString::number(websocketPeak))
        );

    m_labelRotations->setText(
        QString("%1 (%2)")
            .arg(QString::number(ipSucRotations))
            .arg(QString::number(ipRotations))
        );

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
