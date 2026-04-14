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
    // На Linux Qt::Popup игнорирует move() и авто-позиционируется WM.
    // Qt::Tool + WindowStaysOnTopHint позволяет ручное позиционирование.
    // Автоскрытие через FocusOut polling.
#ifdef Q_OS_LINUX
    return Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint;
#else
    return Qt::FramelessWindowHint | Qt::Popup;
#endif
}

bool TrayMenu::event(QEvent* event) {
#ifdef Q_OS_LINUX
    // Linux: Qt::Tool не закрывается сам. Отслеживаем потерю фокуса, чтобы запустить таймер скрытия.
    // Но только если не IPC-режим (там polling управляет скрытием).
    if (!m_ipcMode && (event->type() == QEvent::FocusOut || event->type() == QEvent::WindowDeactivate)) {
        m_autoHideTimer->start();
    }
#endif
    // Windows/macOS: Qt::Popup закрывается системой автоматически при клике вне меню.
    // Не вмешиваемся в этот процесс.
    return QWidget::event(event);
}

void TrayMenu::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // На Linux скрытие управляется через startIpcFocusMonitor.
    // На Windows/macOS Qt::Popup закрывается системой автоматически.
}

void TrayMenu::hideEvent(QHideEvent* event) {
    m_autoHideTimer->stop();
    m_ipcMode = false;
    // Сброс настроек таймера к дефолтным (для Windows)
    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(50);
    QWidget::hideEvent(event);
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
    bool isFocused = hasFocus() || isActiveWindow();

    if (m_ipcMode) {
        // IPC-показ или Linux QMenu: запоминаем, была ли мышь над меню.
        if (underMouse()) {
            m_wasUnderMouse = true;
        }
        
        bool canHide = true;
        if (m_ipcRequireHover && !m_wasUnderMouse) {
            canHide = false; // Не скрываем пока пользователь хотя бы раз не навёл мышь
        }
        
        if (canHide && !underMouse() && !isFocused) {
            hide();
        }
    } else {
        // Обычный показ (клик по иконке): скрываем если мышь не над меню и нет фокуса
        if (!underMouse() && !isFocused) {
            hide();
        }
    }
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

void TrayMenu::startIpcFocusMonitor(bool requireHover) {
    // IPC-показ: после grace period (500 ms) переключаемся на polling.
    // Дожидаемся его завершения и включаем циклический polling (100 ms).
    m_ipcMode = true;
    m_ipcRequireHover = requireHover;
    m_wasUnderMouse = false;
    
    m_autoHideTimer->stop();

    // Ждём завершения grace period (showEvent уже запустил singleShot на 500 ms).
    // После этого переключаемся на polling.
    QTimer::singleShot(500, this, [this]() {
        if (isVisible() && m_ipcMode) {
            m_autoHideTimer->setSingleShot(false);
            m_autoHideTimer->setInterval(100);
            m_autoHideTimer->start();
        }
    });
}
