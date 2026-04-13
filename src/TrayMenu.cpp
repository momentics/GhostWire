#include "TrayMenu.h"
#include "StatsPanel.h"
#include "SparklineWidget.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>

TrayMenu::TrayMenu(QWidget* parent)
    : QWidget(parent, makeWindowFlags())
    , m_autoHideTimer(new QTimer(this))
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowOpacity(0.95);

    // Qt::Popup автоматически закрывается при клике вне области,
    // но на некоторых WM (Linux) это работает нестабильно.
    // Дополнительно ловим FocusOut для надёжного автоскрытия.
    setAttribute(Qt::WA_ShowWithoutActivating, false);

    setFixedWidth(Config::MENU_WIDTH);
    setMaximumWidth(Config::MENU_WIDTH);

    // Таймер автоскрытия: перезапускается при каждом FocusOut,
    // что предотвращает накопление отложенных скрытий
    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(50);
    connect(m_autoHideTimer, &QTimer::timeout, this, &TrayMenu::tryHideMenu);

    buildLayout();
}

Qt::WindowFlags TrayMenu::makeWindowFlags() {
    // Qt::Popup — стандартный флаг для контекстных меню:
    //   • Не появляется на панели задач
    //   • Автоматически закрывается при клике вне области
    //   • Корректный z-order (поверх родительского окна)
    //   • Работает одинаково на Windows 10/11, Linux, macOS
    // Qt::WindowStaysOnTopHint НЕ нужен — Popup уже поверх.
    // На Windows 11 комбинация Tool + WindowStaysOnTopHint ломает
    // foreground activation, из-за чего меню не показывается.
    return Qt::FramelessWindowHint | Qt::Popup;
}

bool TrayMenu::event(QEvent* event) {
    // Скрываем при потере фокуса (клик вне меню)
    // FocusOut надёжнее WindowDeactivate — работает на всех платформах,
    // включая Windows 11, где Tool-окна не получают активное состояние.
    if (event->type() == QEvent::FocusOut) {
        // Перезапуск таймера — предотвращает накопление отложенных скрытий
        m_autoHideTimer->start();
    }
    return QWidget::event(event);
}

void TrayMenu::mousePressEvent(QMouseEvent* event) {
    // Клик на пустое пространство — закрываем меню.
    // Qt::Popup делает это автоматически, но на некоторых Linux WM
    // (например, KDE/KWin) клик без обработчика может не закрыть окно.
    // Проверяем, что клик НЕ по кнопке (дочернему виджету).
    if (!childAt(event->pos())) {
        hide();
    } else {
        QWidget::mousePressEvent(event);
    }
}

TrayMenu::~TrayMenu() = default;

void TrayMenu::tryHideMenu() {
    if (!underMouse() && !hasFocus())
        hide();
}

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
        if (!m_isRunning) {
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

    // Кнопка Проверить обновления
    m_updateButton = new QPushButton(tr("Проверить обновления"), this);
    connect(m_updateButton, &QPushButton::clicked, this, [this]() {
        emit checkUpdatesRequested();
    });
    mainLayout->addWidget(m_updateButton);

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
    m_isRunning = running;
    if (m_toggleButton) {
        m_toggleButton->setText(running ? tr("Стоп") : tr("Старт"));
    }
}

void TrayMenu::hideMenu() {
    hide();
}
