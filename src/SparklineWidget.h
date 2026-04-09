#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>

/// Спарклайн — компактный линейный график RX/TX.
/// - Ось X: последние значения, старые слева, новые справа (сдвиг)
/// - Ось Y: autoscale по текущим данным
/// - Сетка: горизонтальные линии + подписи значений
class SparklineWidget : public QWidget {
    Q_OBJECT
public:
    explicit SparklineWidget(QWidget* parent = nullptr);

    /// Добавить новое значение (байты за последний интервал опроса)
    void addPoint(double rx, double tx);

    /// Очистить историю
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_rx;
    QVector<double> m_tx;
    QVector<QPointF> m_rxPoints;
    QVector<QPointF> m_txPoints;
    int m_maxPoints = 60;

    QColor m_rxColor;
    QColor m_txColor;
    QColor m_gridColor;
    QColor m_textColor;

    QFont m_labelFont;
    QFont m_legendFont;

    void drawGrid(QPainter& painter, double maxVal);
    void drawTimeLabels(QPainter& painter);
    void drawSeries(QPainter& painter, const QVector<double>& data, const QColor& color, double maxVal);
    void drawYLabels(QPainter& painter, double maxVal);
    void drawLegend(QPainter& painter);
    QString formatBytes(double bytesPerSec) const;
    void updatePointsCache(double maxVal);
};
