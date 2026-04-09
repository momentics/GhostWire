#include "SparklineWidget.h"
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

    update();
}

void SparklineWidget::clear() {
    m_rx.clear();
    m_tx.clear();
    update();
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
        painter.drawText(chartRect, Qt::AlignCenter, "нет данных");
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
    drawSeries(painter, m_rx, m_rxColor);
    drawSeries(painter, m_tx, m_txColor);
    drawLegend(painter);
}

void SparklineWidget::drawGrid(QPainter& painter, double /*maxVal*/) {
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
    QFont font = painter.font();
    font.setPointSize(6);
    painter.setFont(font);
    QFontMetrics fm(font);

    // Метки времени: 30м, 15м, сейчас
    const char* labels[] = { "30м", "15м", "сейчас" };
    int positions[] = { 0, 2, 4 };  // индексы для 5 делений сетки
    int w = width();
    int padLeft = 22;
    int padRight = 8;
    int gridW = w - padLeft - padRight;

    for (int i = 0; i < 3; ++i) {
        double frac = static_cast<double>(positions[i]) / 4.0;
        int x = padLeft + static_cast<int>(frac * gridW);
        QString label(labels[i]);
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
    QFont font = painter.font();
    font.setPointSize(6);
    painter.setFont(font);

    // Только верхняя метка (maxVal)
    QString label = formatBytes(maxVal);
    painter.drawText(2, padTop + 4, label);
}

void SparklineWidget::drawSeries(QPainter& painter, const QVector<double>& series, const QColor& color) {

    // Определяем autoscale maxVal по обоим рядам
    double maxVal = 0;
    for (double v : m_rx) if (v > maxVal) maxVal = v;
    for (double v : m_tx) if (v > maxVal) maxVal = v;
    if (maxVal < 1.0) maxVal = 1.0;

    double magnitude = std::pow(10, std::floor(std::log10(maxVal)));
    double norm = maxVal / magnitude;
    if (norm > 5)      maxVal = 10 * magnitude;
    else if (norm > 2) maxVal = 5  * magnitude;
    else if (norm > 1) maxVal = 2  * magnitude;

    int w = width();
    int h = height();
    int padTop = 18;
    int padBot = 14;  // Место для временных меток
    int padLeft = 22;
    int padRight = 8;
    int gridH = h - padTop - padBot;
    int gridW = w - padLeft - padRight;

    QPen pen(color, 1.5);
    painter.setPen(pen);

    // Точки: слева (самые старые) -> справа (самые новые)
    // Если точек меньше m_maxPoints — начинаем с левого края
    int n = series.size();
    QVector<QPointF> points;
    for (int i = 0; i < n; ++i) {
        // frac: 0 = левый край, 1 = правый край
        // Все точки прижаты к правому краю (новые справа)
        int totalSlots = m_maxPoints;
        int offset = totalSlots - n;  // сдвиг слева
        double frac = static_cast<double>(i + offset) / static_cast<double>(totalSlots - 1);
        double x = padLeft + frac * gridW;
        double y = padTop + gridH - (series[i] / maxVal) * gridH;
        points.append(QPointF(x, y));
    }

    painter.drawPolyline(points.constData(), points.size());
}

QString SparklineWidget::formatBytes(double bytesPerSec) const {
    if (bytesPerSec < 1024)
        return QString("%1 Б").arg(static_cast<int>(bytesPerSec));
    if (bytesPerSec < 1024 * 1024)
        return QString("%1 К").arg(bytesPerSec / 1024, 0, 'f', 0);
    if (bytesPerSec < 1024 * 1024 * 1024)
        return QString("%1 М").arg(bytesPerSec / (1024 * 1024), 0, 'f', 1);
    return QString("%1 Г").arg(bytesPerSec / (1024 * 1024 * 1024), 0, 'f', 1);
}

void SparklineWidget::drawLegend(QPainter& painter) {
    int w = width();

    QFont font = painter.font();
    font.setPointSize(6);
    painter.setFont(font);
    QFontMetrics fm(font);

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
