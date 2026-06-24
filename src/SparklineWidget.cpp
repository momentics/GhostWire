#include "SparklineWidget.h"
#include "Config.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFontMetrics>
#include <QApplication>
#include <QtGlobal>
#include <cmath>

namespace {

enum class ScaleUnit {
    Bytes,
    KiB,
    MiB,
    GiB,
    TiB
};

struct ScaleValue {
    double bytes = 1.0;
    double unit = 1.0;
    ScaleUnit label = ScaleUnit::Bytes;
};

ScaleValue calculateScale(double maxBytes) {
    static constexpr double KiB = 1024.0;
    static constexpr double MiB = KiB * 1024.0;
    static constexpr double GiB = MiB * 1024.0;
    static constexpr double TiB = GiB * 1024.0;

    ScaleValue scale;
    if (maxBytes >= TiB) {
        scale.unit = TiB;
        scale.label = ScaleUnit::TiB;
    } else if (maxBytes >= GiB) {
        scale.unit = GiB;
        scale.label = ScaleUnit::GiB;
    } else if (maxBytes >= MiB) {
        scale.unit = MiB;
        scale.label = ScaleUnit::MiB;
    } else if (maxBytes >= KiB) {
        scale.unit = KiB;
        scale.label = ScaleUnit::KiB;
    }

    const double scaledMax = qMax(maxBytes / scale.unit, 1.0);
    scale.bytes = std::ceil(scaledMax) * scale.unit;
    return scale;
}

QString formatScaleLabel(const ScaleValue& scale) {
    const qint64 value = static_cast<qint64>(std::ceil(scale.bytes / scale.unit));
    switch (scale.label) {
    case ScaleUnit::TiB:
        return QObject::tr("%1 ТБ").arg(value);
    case ScaleUnit::GiB:
        return QObject::tr("%1 ГБ").arg(value);
    case ScaleUnit::MiB:
        return QObject::tr("%1 МБ").arg(value);
    case ScaleUnit::KiB:
        return QObject::tr("%1 КБ").arg(value);
    case ScaleUnit::Bytes:
        return QObject::tr("%1 Б").arg(value);
    }
    return QObject::tr("%1 Б").arg(value);
}

} // namespace

SparklineWidget::SparklineWidget(QWidget* parent)
    : QWidget(parent)
    , m_maxPoints(Config::SPARKLINE_MAX_POINTS)
    , m_writeIndex(0)
    , m_count(0)
{
    m_rx.resize(m_maxPoints);
    m_tx.resize(m_maxPoints);
    m_rxColor   = QColor(80, 160, 255, 180);   // синий полупрозрачный
    m_txColor   = QColor(255, 120, 80, 180);   // оранжевый полупрозрачный
    m_gridColor = QColor(255, 255, 255, 30);   // очень прозрачная сетка
    m_textColor = QColor(180, 180, 180, 200);  // серый текст

    // Размер в пунктах сохраняет читаемость подписей при дробном масштабе экрана.
    m_labelFont = QFont(QApplication::font());
    m_labelFont.setPointSizeF(5.25);

    m_legendFont = QFont(QApplication::font());
    m_legendFont.setPointSizeF(5.25);

    setMinimumHeight(100);
    setMaximumHeight(140);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SparklineWidget::addPoint(double rx, double tx) {
    m_rx[m_writeIndex] = rx;
    m_tx[m_writeIndex] = tx;
    if (m_count < m_maxPoints) ++m_count;
    m_writeIndex = (m_writeIndex + 1) % m_maxPoints;

    m_rxPoints.clear();
    m_txPoints.clear();

    update();
}

void SparklineWidget::clear() {
    m_count = 0;
    m_writeIndex = 0;
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
    painter.setBrush(QColor(30, 30, 30, 200));
    painter.drawRoundedRect(chartRect, 4, 4);

    if (m_count < 2) {
        painter.setPen(m_textColor);
        painter.drawText(chartRect, Qt::AlignCenter, tr("нет данных"));
        return;
    }

    double maxSample = 0;
    for (int i = 0; i < m_count; ++i) {
        int idx = (m_writeIndex - m_count + i + m_maxPoints) % m_maxPoints;
        if (m_rx[idx] > maxSample) maxSample = m_rx[idx];
        if (m_tx[idx] > maxSample) maxSample = m_tx[idx];
    }

    // Минимальный масштаб чтобы не делить на ноль
    if (maxSample < 1.0) maxSample = 1.0;

    const ScaleValue scale = calculateScale(maxSample);
    const double maxVal = scale.bytes;
    drawGrid(painter, maxVal);
    drawTimeLabels(painter);
    drawYLabels(painter, formatScaleLabel(scale));
    updatePointsCache(maxVal);

    // Ограничиваем рисование серий областью сетки, чтобы сплайн не выходил за границы
    int gridH = height() - PAD_TOP - PAD_BOT;
    int gridW = width() - PAD_LEFT - PAD_RIGHT;
    QRectF gridClipRect(PAD_LEFT, PAD_TOP, gridW, gridH);
    painter.save();
    painter.setClipRect(gridClipRect);

    double sumRx = 0.0;
    double sumTx = 0.0;
    for (int i = 0; i < m_count; ++i) {
        int idx = (m_writeIndex - m_count + i + m_maxPoints) % m_maxPoints;
        sumRx += m_rx[idx];
        sumTx += m_tx[idx];
    }

    if (sumRx > sumTx) {
        drawSeries(painter, m_rx, m_rxColor);
        drawSeries(painter, m_tx, m_txColor);
    } else {
        drawSeries(painter, m_tx, m_txColor);
        drawSeries(painter, m_rx, m_rxColor);
    }
    painter.restore();
    drawLegend(painter);
}

void SparklineWidget::drawGrid(QPainter& painter, double) {
    int gridH = height() - PAD_TOP - PAD_BOT;
    int gridW = width() - PAD_LEFT - PAD_RIGHT;

    // Горизонтальные линии (5 основных)
    painter.setPen(QPen(m_gridColor, 0.5));
    for (int i = 0; i <= 4; ++i) {
        int y = PAD_TOP + (gridH * i) / 4;
        painter.drawLine(PAD_LEFT, y, PAD_LEFT + gridW, y);
    }

    // Промежуточные горизонтальные линии (4 тонкие)
    painter.setPen(QPen(m_gridColor, 0.3));
    for (int i = 0; i < 4; ++i) {
        int y = PAD_TOP + (gridH * (i * 2 + 1)) / 8;
        painter.drawLine(PAD_LEFT, y, PAD_LEFT + gridW, y);
    }

    // Вертикальные линии
    int numPoints = m_count;
    if (numPoints < 2) return;

    painter.setPen(QPen(m_gridColor, 0.8));
    for (int i = 0; i <= 4; ++i) {
        double frac = static_cast<double>(i) / 4.0;
        int x = PAD_LEFT + static_cast<int>(frac * gridW);
        painter.drawLine(x, PAD_TOP, x, PAD_TOP + gridH);
    }
}

void SparklineWidget::drawTimeLabels(QPainter& painter) {
    painter.setPen(m_textColor);
    painter.setFont(m_labelFont);
    QFontMetrics fm(m_labelFont);

    // Метки времени: 30м, 15м, сейчас
    const char* labels[] = { QT_TR_NOOP("30м"), QT_TR_NOOP("15м"), QT_TR_NOOP("сейчас") };
    int positions[] = { 0, 2, 4 };
    int gridW = width() - PAD_LEFT - PAD_RIGHT;

    for (int i = 0; i < 3; ++i) {
        double frac = static_cast<double>(positions[i]) / 4.0;
        int x = PAD_LEFT + static_cast<int>(frac * gridW);
        QString label = tr(labels[i]);
        int labelW = fm.horizontalAdvance(label);

        // Центрируем метку по позиции деления
        int drawX = x - labelW / 2;

        // Не даём вылезти за края
        if (drawX < 0) drawX = 0;
        if (drawX + labelW > width()) drawX = width() - labelW;

        painter.drawText(drawX, height() - PAD_BOT + fm.ascent(), label);
    }
}

void SparklineWidget::drawYLabels(QPainter& painter, const QString& label) {
    painter.setPen(m_textColor);
    painter.setFont(m_labelFont);
    QFontMetrics fm(m_labelFont);

    // Метка в области отступа слева
    int labelW = fm.horizontalAdvance(label);
    int x = PAD_LEFT - labelW - 4;
    if (x < 0) x = 0;
    painter.drawText(x, PAD_TOP + 4, label);
}

void SparklineWidget::drawSeries(QPainter& painter, const QVector<double>& series, const QColor& color) {
    const QVector<QPointF>& points = (series == m_rx) ? m_rxPoints : m_txPoints;
    if (points.isEmpty()) return;

    int gridH = height() - PAD_TOP - PAD_BOT;
    double baseline = PAD_TOP + gridH;

    // Gradient fill: full color at top, transparent at bottom
    QLinearGradient grad(0, PAD_TOP, 0, baseline);
    grad.setColorAt(0.0, QColor(color.red(), color.green(), color.blue(), color.alpha()));
    grad.setColorAt(1.0, QColor(color.red(), color.green(), color.blue(), 0));

    // Build smooth filled area using Catmull-Rom spline
    QPainterPath areaPath;
    areaPath.moveTo(points.first());

    int n = points.size();
    for (int i = 0; i < n - 1; ++i) {
        int prev = qMax(i - 1, 0);
        int next = qMin(i + 2, n - 1);
        const QPointF& p0 = points[prev];
        const QPointF& p1 = points[i];
        const QPointF& p2 = points[i + 1];
        const QPointF& p3 = points[next];

        QPointF cp1(p1.x() + (p2.x() - p0.x()) / 6.0,
                    p1.y() + (p2.y() - p0.y()) / 6.0);
        QPointF cp2(p2.x() - (p3.x() - p1.x()) / 6.0,
                    p2.y() - (p3.y() - p1.y()) / 6.0);

        areaPath.cubicTo(cp1, cp2, p2);
    }

    areaPath.lineTo(points.last().x(), baseline);
    areaPath.lineTo(points.first().x(), baseline);
    areaPath.closeSubpath();

    painter.setPen(Qt::NoPen);
    painter.setBrush(grad);
    painter.drawPath(areaPath);

    // Smooth line on top
    QPainterPath linePath;
    linePath.moveTo(points.first());

    for (int i = 0; i < n - 1; ++i) {
        int prev = qMax(i - 1, 0);
        int next = qMin(i + 2, n - 1);
        const QPointF& p0 = points[prev];
        const QPointF& p1 = points[i];
        const QPointF& p2 = points[i + 1];
        const QPointF& p3 = points[next];

        QPointF cp1(p1.x() + (p2.x() - p0.x()) / 6.0,
                    p1.y() + (p2.y() - p0.y()) / 6.0);
        QPointF cp2(p2.x() - (p3.x() - p1.x()) / 6.0,
                    p2.y() - (p3.y() - p1.y()) / 6.0);

        linePath.cubicTo(cp1, cp2, p2);
    }

    painter.setPen(QPen(color, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(linePath);
}

void SparklineWidget::updatePointsCache(double maxVal) {
    int gridH = height() - PAD_TOP - PAD_BOT;
    int gridW = width() - PAD_LEFT - PAD_RIGHT;

    auto computePoints = [this, gridW, gridH, maxVal](const QVector<double>& series) -> QVector<QPointF> {
        QVector<QPointF> pts;
        if (m_count < 2) return pts;

        pts.reserve(m_count);
        int offset = m_maxPoints - m_count;
        for (int i = 0; i < m_count; ++i) {
            int idx = (m_writeIndex - m_count + i + m_maxPoints) % m_maxPoints;
            double frac = static_cast<double>(i + offset) / static_cast<double>(m_maxPoints - 1);
            double x = PAD_LEFT + frac * gridW;
            double y = PAD_TOP + gridH - (series[idx] / maxVal) * gridH;
            pts.append(QPointF(x, y));
        }
        return pts;
    };

    if (m_rxPoints.isEmpty())
        m_rxPoints = computePoints(m_rx);
    if (m_txPoints.isEmpty())
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
