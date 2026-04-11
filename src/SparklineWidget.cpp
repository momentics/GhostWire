#include "SparklineWidget.h"
#include "Utils.h"
#include "Config.h"
#include <QPainter>
#include <QFontMetrics>
#include <cmath>

SparklineWidget::SparklineWidget(QWidget* parent)
    : QWidget(parent)
    , m_maxPoints(Config::SPARKLINE_MAX_POINTS)
{
    m_rxColor   = QColor(80, 160, 255, 180);   // синий полупрозрачный
    m_txColor   = QColor(255, 120, 80, 180);   // оранжевый полупрозрачный
    m_gridColor = QColor(255, 255, 255, 30);   // очень прозрачная сетка
    m_textColor = QColor(180, 180, 180, 200);  // серый текст

    // Шрифты создаём один раз
    m_labelFont = font();
    m_labelFont.setPointSize(6);

    m_legendFont = font();
    m_legendFont.setPointSize(6);

    setMinimumHeight(100);
    setMaximumHeight(140);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SparklineWidget::addPoint(double rx, double tx) {
    m_rx.append(rx);
    m_tx.append(tx);

    // Храним не больше m_maxPoints точек
    while (m_rx.size() > m_maxPoints) m_rx.removeFirst();
    while (m_tx.size() > m_maxPoints) m_tx.removeFirst();

    // Сбрасываем кеш точек — пересчитается при следующем paint
    m_rxPoints.clear();
    m_txPoints.clear();

    update();
}

void SparklineWidget::clear() {
    m_rx.clear();
    m_tx.clear();
    m_rxPoints.clear();
    m_txPoints.clear();
    update();
}

void SparklineWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // Инвалидируем кэш точек — координаты зависят от размеров виджета
    m_rxPoints.clear();
    m_txPoints.clear();
}

void SparklineWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Затемненная подложка
    QRect chartRect = rect().adjusted(2, 2, -2, -2);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(40, 40, 40, 180));
    painter.drawRoundedRect(chartRect, 4, 4);

    if (m_rx.size() < 2 && m_tx.size() < 2) {
        // Нет данных — рисуем заглушку
        painter.setPen(m_textColor);
        painter.drawText(chartRect, Qt::AlignCenter, tr("нет данных"));
        return;
    }

    // Определяем autoscale maxVal по обоим рядам
    double maxVal = 0;
    for (double v : m_rx) if (v > maxVal) maxVal = v;
    for (double v : m_tx) if (v > maxVal) maxVal = v;

    // Минимальный масштаб чтобы не делить на ноль
    if (maxVal < 1.0) maxVal = 1.0;

    // Округляем maxVal до красивой цифры
    double magnitude = std::pow(10, std::floor(std::log10(maxVal)));
    double norm = maxVal / magnitude;
    if (norm > 5)      maxVal = 10 * magnitude;
    else if (norm > 2) maxVal = 5  * magnitude;
    else if (norm > 1) maxVal = 2  * magnitude;

    drawGrid(painter, maxVal);
    drawTimeLabels(painter);
    drawYLabels(painter, maxVal);
    updatePointsCache(maxVal);
    drawSeries(painter, m_rx, m_rxColor, maxVal);
    drawSeries(painter, m_tx, m_txColor, maxVal);
    drawLegend(painter);
}

void SparklineWidget::drawGrid(QPainter& painter, double) {
    int w = width();
    int h = height();
    int padTop = 18;
    int padBot = 14;  // Место для временных меток
    int padLeft = 22; // Место для метки "500 К" и прочего
    int padRight = 8; // Запас справа

    int gridH = h - padTop - padBot;
    int gridW = w - padLeft - padRight;

    // Горизонтальные линии (4 уровня)
    painter.setPen(QPen(m_gridColor, 0.5));
    for (int i = 0; i <= 4; ++i) {
        int y = padTop + (gridH * i) / 4;
        painter.drawLine(padLeft, y, padLeft + gridW, y);
    }

    // Вертикальные линии — заметнее
    int numPoints = qMax(m_rx.size(), m_tx.size());
    if (numPoints < 2) return;

    painter.setPen(QPen(m_gridColor, 0.8));
    for (int i = 0; i <= 4; ++i) {
        double frac = static_cast<double>(i) / 4.0;
        int x = padLeft + static_cast<int>(frac * gridW);
        painter.drawLine(x, padTop, x, padTop + gridH);
    }
}

void SparklineWidget::drawTimeLabels(QPainter& painter) {
    int h = height();
    int padBot = 14;

    painter.setPen(m_textColor);
    painter.setFont(m_labelFont);
    QFontMetrics fm(m_labelFont);

    // Метки времени: 30м, 15м, сейчас
    const char* labels[] = { QT_TR_NOOP("30м"), QT_TR_NOOP("15м"), QT_TR_NOOP("сейчас") };
    int positions[] = { 0, 2, 4 };  // индексы для 5 делений сетки
    int w = width();
    int padLeft = 22;
    int padRight = 8;
    int gridW = w - padLeft - padRight;

    for (int i = 0; i < 3; ++i) {
        double frac = static_cast<double>(positions[i]) / 4.0;
        int x = padLeft + static_cast<int>(frac * gridW);
        QString label = tr(labels[i]);
        int labelW = fm.horizontalAdvance(label);

        // Центрируем метку по позиции деления
        int drawX = x - labelW / 2;

        // Но не даем вылезти за края
        if (drawX < 0) drawX = 0;
        if (drawX + labelW > w) drawX = w - labelW;

        painter.drawText(drawX, h - padBot + fm.ascent(), label);
    }
}

void SparklineWidget::drawYLabels(QPainter& painter, double maxVal) {
    int padTop = 18;

    painter.setPen(m_textColor);
    painter.setFont(m_labelFont);

    // Только верхняя метка (maxVal)
    QString label = formatBytes(maxVal);
    painter.drawText(2, padTop + 4, label);
}

void SparklineWidget::drawSeries(QPainter& painter, const QVector<double>& series, const QColor& color, double) {
    // Используем кешированные точки
    const QVector<QPointF>& points = (series == m_rx) ? m_rxPoints : m_txPoints;
    if (points.isEmpty()) return;

    QPen pen(color, 1.5);
    painter.setPen(pen);
    painter.drawPolyline(points.constData(), points.size());
}

void SparklineWidget::updatePointsCache(double maxVal) {
    int w = width();
    int h = height();
    int padTop = 18;
    int padBot = 14;
    int padLeft = 22;
    int padRight = 8;
    int gridH = h - padTop - padBot;
    int gridW = w - padLeft - padRight;

    auto computePoints = [padLeft, padRight, gridW, gridH, padTop, maxVal, this]
                         (const QVector<double>& series) -> QVector<QPointF> {
        QVector<QPointF> pts;
        int n = series.size();
        if (n < 2) return pts;

        int offset = m_maxPoints - n;
        pts.reserve(n);
        for (int i = 0; i < n; ++i) {
            double frac = static_cast<double>(i + offset) / static_cast<double>(m_maxPoints - 1);
            double x = padLeft + frac * gridW;
            double y = padTop + gridH - (series[i] / maxVal) * gridH;
            pts.append(QPointF(x, y));
        }
        return pts;
    };

    if (m_rxPoints.isEmpty() && !m_rx.isEmpty())
        m_rxPoints = computePoints(m_rx);
    if (m_txPoints.isEmpty() && !m_tx.isEmpty())
        m_txPoints = computePoints(m_tx);
}


void SparklineWidget::drawLegend(QPainter& painter) {
    int w = width();

    painter.setFont(m_legendFont);
    QFontMetrics fm(m_legendFont);

    int lineLen = 12;
    int dotR = 2;
    int spacing = 8;

    // Ширина одного элемента: линия + точка + текст
    int itemW = lineLen + dotR * 2 + 4 + fm.horizontalAdvance("RX");
    // Два элемента + отступ между ними
    int legendW = itemW * 2 + spacing;
    int legendH = fm.height() + 2;

    int legendX = (w - legendW) / 2;
    int legendY = 1;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 30, 30, 200));
    painter.drawRoundedRect(legendX - 2, legendY, legendW + 4, legendH, 3, 3);

    int y = legendY + dotR + 2;

    // RX (левая часть)
    int x1 = legendX;
    QPen rxPen(m_rxColor, 1.5);
    painter.setPen(rxPen);
    painter.drawLine(x1, y, x1 + lineLen, y);
    painter.setBrush(m_rxColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(x1 + lineLen + 1, y - dotR, dotR * 2, dotR * 2);
    painter.setPen(m_textColor);
    painter.drawText(x1 + lineLen + dotR * 2 + 3, y + fm.ascent() / 2 + 1, "RX");

    // TX (правая часть)
    int x2 = x1 + itemW + spacing;
    QPen txPen(m_txColor, 1.5);
    painter.setPen(txPen);
    painter.drawLine(x2, y, x2 + lineLen, y);
    painter.setBrush(m_txColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(x2 + lineLen + 1, y - dotR, dotR * 2, dotR * 2);
    painter.setPen(m_textColor);
    painter.drawText(x2 + lineLen + dotR * 2 + 3, y + fm.ascent() / 2 + 1, "TX");
}
