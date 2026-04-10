#include "TrayMenu.h"
#include "StatsPanel.h"
#include "SparklineWidget.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QPointer>

TrayMenu::TrayMenu(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowOpacity(0.95);

    setFixedWidth(Config::MENU_WIDTH);
    setMaximumWidth(Config::MENU_WIDTH);

    buildLayout();
}

bool TrayMenu::event(QEvent* event) {
    // Скрываем при потере фокуса (клик вне меню)
    if (event->type() == QEvent::WindowDeactivate) {
        // QPointer гарантирует безопасность: если объект уничтожен, лямбда не выполнится
        QPointer<TrayMenu> safeThis(this);
        QTimer::singleShot(50, this, [safeThis]() {
            if (safeThis && !safeThis->underMouse() && !safeThis->hasFocus())
                safeThis->hide();
        });
    }
    return QWidget::event(event);
}

TrayMenu::~TrayMenu() = default;

void TrayMenu::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Стиль: похож на контекстное меню
    setStyleSheet(R"(
        TrayMenu {
            background-color: #2b2b2b;
            border: 1px solid #555;
            border-radius: 6px;
        }
        QLabel {
            color: #ddd;
            padding: 2px 0;
        }
        QPushButton {
            color: #ddd;
            background-color: transparent;
            border: none;
            border-radius: 4px;
            padding: 6px 16px;
            text-align: left;
        }
        QPushButton:hover {
            background-color: #3a3a3a;
        }
        QPushButton:pressed {
            background-color: #444;
        }
    )");

    // Статистика
    m_statsPanel = new StatsPanel(this);
    mainLayout->addWidget(m_statsPanel);

    // Спарклайн
    m_sparkline = new SparklineWidget(this);
    mainLayout->addWidget(m_sparkline);

    // Разделитель
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("QFrame { background-color: #555; margin: 0 6px; }");
    mainLayout->addWidget(line);

    // Кнопка Старт/Стоп
    m_toggleButton = new QPushButton(tr("Старт"), this);
    connect(m_toggleButton, &QPushButton::clicked, this, [this]() {
        if (m_toggleButton->text() == tr("Старт")) {
            emit startRequested();
        } else {
            emit stopRequested();
        }
    });
    mainLayout->addWidget(m_toggleButton);

    // Разделитель 1
    //auto* line1 = new QFrame(this);
    //line1->setFrameShape(QFrame::HLine);
    //line1->setFrameShadow(QFrame::Sunken);
    //line1->setStyleSheet("QFrame { background-color: #555; margin: 0 6px; }");
    //mainLayout->addWidget(line1);

    // Кнопка Подключить Telegram
    m_telegramButton = new QPushButton(tr("Подключить Telegram"), this);
    connect(m_telegramButton, &QPushButton::clicked, this, [this]() {
        emit configureTelegramRequested();
    });
    mainLayout->addWidget(m_telegramButton);

    // Разделитель 2
    //auto* line2 = new QFrame(this);
    //line2->setFrameShape(QFrame::HLine);
    //line2->setFrameShadow(QFrame::Sunken);
    //line2->setStyleSheet("QFrame { background-color: #555; margin: 0 6px; }");
    //mainLayout->addWidget(line2);

    // Кнопка Выход
    m_exitButton = new QPushButton(tr("Выход"), this);
    connect(m_exitButton, &QPushButton::clicked, this, [this]() {
        emit exitRequested();
    });
    mainLayout->addWidget(m_exitButton);
}

void TrayMenu::setStats(uint64_t uptimeSecs, uint64_t websocketActive,
                        double peakRx, double peakTx,
                        uint64_t totalRx, uint64_t totalTx) {
    if (m_statsPanel)
        m_statsPanel->updateStats(uptimeSecs, websocketActive, peakRx, peakTx, totalRx, totalTx);
}

void TrayMenu::addSparklinePoint(double rxBytesPerSec, double txBytesPerSec) {
    if (m_sparkline)
        m_sparkline->addPoint(rxBytesPerSec, txBytesPerSec);
}

void TrayMenu::clearSparkline() {
    if (m_sparkline)
        m_sparkline->clear();
}

void TrayMenu::setRunningState(bool running) {
    if (m_toggleButton) {
        m_toggleButton->setText(running ? tr("Стоп") : tr("Старт"));
    }
}

void TrayMenu::hideMenu() {
    hide();
}
