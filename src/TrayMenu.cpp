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
    , m_ipcTimeoutTimer(new QTimer(this))
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowOpacity(0.95);

    setFixedWidth(Config::MENU_WIDTH);
    setMaximumWidth(Config::MENU_WIDTH);

    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(50);
    connect(m_autoHideTimer, &QTimer::timeout, this, &TrayMenu::tryHideMenu);

    m_ipcTimeoutTimer->setSingleShot(true);
    m_ipcTimeoutTimer->setInterval(3000); // 3-секундный таймер
    connect(m_ipcTimeoutTimer, &QTimer::timeout, this, [this]() {
        // Срабатывает, если был запущен второй экземпляр, но пользователь не пошевелил мышью
        // Если мышь не над меню и фокуса нет, скрываем
        if (!underMouse() && (qApp->applicationState() != Qt::ApplicationActive)) {
            hide();
        }
    });

    buildLayout();
}

Qt::WindowFlags TrayMenu::makeWindowFlags() {
#ifdef Q_OS_LINUX
    return Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint;
#else
    return Qt::FramelessWindowHint | Qt::Popup;
#endif
}

bool TrayMenu::event(QEvent* event) {
    if (event->type() == QEvent::WindowDeactivate) {
        if (!m_ipcMode) m_autoHideTimer->start();
    }
    return QWidget::event(event);
}

void TrayMenu::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
}

void TrayMenu::hideEvent(QHideEvent* event) {
    m_autoHideTimer->stop();
    m_ipcMode = false;
    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(50);
    QWidget::hideEvent(event);
}

void TrayMenu::mousePressEvent(QMouseEvent* event) {
    if (!childAt(event->pos())) {
        hide();
    } else {
        QWidget::mousePressEvent(event);
    }
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

void TrayMenu::tryHideMenu() {
    bool isAppActive = (qApp->applicationState() == Qt::ApplicationActive);
    bool isFocused = hasFocus() || isActiveWindow() || isAppActive;

    if (m_ipcMode) {
        if (underMouse()) {
            m_wasUnderMouse = true;
            m_ipcTimeoutTimer->stop(); // Пользователь навел мышь, отменяем 3-секундный таймер
        }

        bool canHide = true;
        if (m_ipcRequireHover && !m_wasUnderMouse) {
            canHide = false;
        }

        // Если приложение потеряло фокус оконного менеджера (клик вне приложения),
        // qApp->applicationState() станет Inactive. Тогда isFocused = false.
        if (!isFocused || (canHide && !underMouse() && !isAppActive)) {
            hide();
        }
    } else {
        if (!underMouse() && !isFocused) {
            hide();
        }
    }
}

void TrayMenu::startIpcFocusMonitor(bool requireHover, bool autoHideTimeout) {
    m_ipcMode = true;
    m_ipcRequireHover = requireHover;
    m_wasUnderMouse = false;
    m_autoHideTimer->stop();
    m_ipcTimeoutTimer->stop();

    if (autoHideTimeout) {
        m_ipcTimeoutTimer->start();
    }

    QTimer::singleShot(250, this, [this]() {
        if (isVisible() && m_ipcMode) {
            m_autoHideTimer->setSingleShot(false);
            m_autoHideTimer->setInterval(100);
            m_autoHideTimer->start();
        }
    });
}
