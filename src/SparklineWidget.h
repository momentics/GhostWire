#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>

class QString;

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
    void resizeEvent(QResizeEvent* event) override;

private:
    // Отступы области графика
    static constexpr int PAD_TOP    = 18;
    static constexpr int PAD_BOT    = 14;
    static constexpr int PAD_LEFT   = 22;
    static constexpr int PAD_RIGHT  = 8;

    QVector<double> m_rx;
    QVector<double> m_tx;
    QVector<QPointF> m_rxPoints;
    QVector<QPointF> m_txPoints;
    int m_maxPoints;

    QColor m_rxColor;
    QColor m_txColor;
    QColor m_gridColor;
    QColor m_textColor;

    QFont m_labelFont;
    QFont m_legendFont;

    void drawGrid(QPainter& painter, double maxVal);
    void drawTimeLabels(QPainter& painter);
    void drawSeries(QPainter& painter, const QVector<double>& data, const QColor& color);
    void drawYLabels(QPainter& painter, const QString& label);
    void drawLegend(QPainter& painter);
    void updatePointsCache(double maxVal);
};
