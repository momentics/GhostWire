#include "TrayMenu.h"
#include "StatsPanel.h"
#include "SparklineWidget.h"
#include "Config.h"
#ifdef Q_OS_MAC
#include "MacPlatformIntegration.h"
#endif
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>
#include <QScopedValueRollback>

namespace {
bool isDarkPalette(const QPalette& palette) {
    return palette.color(QPalette::Window).lightness() < 128;
}

QString separatorStyle() {
#ifdef Q_OS_MAC
    return isDarkPalette(QApplication::palette())
        ? QStringLiteral("QFrame { background-color: rgba(235, 235, 245, 36); margin: 2px 0; max-height: 1px; }")
        : QStringLiteral("QFrame { background-color: rgba(60, 60, 67, 44); margin: 2px 0; max-height: 1px; }");
#else
    return QStringLiteral("QFrame { background-color: #555; margin: 0 6px; }");
#endif
}
} // namespace

TrayMenu::TrayMenu(QWidget* parent)
    : QWidget(parent, makeWindowFlags())
    , m_autoHideTimer(std::make_unique<QTimer>(this))
    , m_ipcTimeoutTimer(std::make_unique<QTimer>(this))
{
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setWindowOpacity(1.0);
#else
    setAttribute(Qt::WA_TranslucentBackground, false);
    setWindowOpacity(0.95);
#endif
    setAttribute(Qt::WA_DeleteOnClose, false);

#ifdef Q_OS_MAC
    setFixedWidth(320);
    setMaximumWidth(320);
#else
    setFixedWidth(Config::MENU_WIDTH);
    setMaximumWidth(Config::MENU_WIDTH);
#endif

    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(50);
    connect(m_autoHideTimer.get(), &QTimer::timeout, this, &TrayMenu::tryHideMenu);

    m_ipcTimeoutTimer->setSingleShot(true);
    m_ipcTimeoutTimer->setInterval(3000); // 3-секундный таймер
    connect(m_ipcTimeoutTimer.get(), &QTimer::timeout, this, [this]() {
        // Срабатывает, если был запущен второй экземпляр, но пользователь не пошевелил мышью
        // Если мышь не над меню и фокуса нет, скрываем
        if (!underMouse() && (qApp->applicationState() != Qt::ApplicationActive)) {
            hideMenu();
        }
    });

    buildLayout();
}

Qt::WindowFlags TrayMenu::makeWindowFlags() {
#ifdef Q_OS_LINUX
    return Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint;
#elif defined(Q_OS_MAC)
    return Qt::Widget;
#else
    return Qt::FramelessWindowHint | Qt::Popup;
#endif
}

bool TrayMenu::event(QEvent* event) {
    if (event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::PaletteChange) {
        applyPlatformStyle();
    }

    if (event->type() == QEvent::WindowDeactivate) {
        if (!m_ipcMode) m_autoHideTimer->start();
    }
    return QWidget::event(event);
}

void TrayMenu::hideEvent(QHideEvent* event) {
    m_autoHideTimer->stop();
    m_ipcMode = false;
    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(50);
    QWidget::hideEvent(event);
}

void TrayMenu::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
#ifdef Q_OS_MAC
    MacPlatformIntegration::preparePopoverWindow(this);
#endif
}

void TrayMenu::mousePressEvent(QMouseEvent* event) {
    if (!childAt(event->pos())) {
        hideMenu();
    } else {
        QWidget::mousePressEvent(event);
    }
}

TrayMenu::~TrayMenu() {
#ifdef Q_OS_MAC
    MacPlatformIntegration::detachPopoverContent(this);
#endif
}

void TrayMenu::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
#ifdef Q_OS_MAC
    mainLayout->setContentsMargins(12, 10, 12, 12);
    mainLayout->setSpacing(8);
#else
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
#endif

    // Используем размер шрифта в пунктах, чтобы всплывающее меню следовало системному масштабу DPI.
    applyPlatformStyle();

    // Статистика
    m_statsPanel = new StatsPanel(this);
    mainLayout->addWidget(m_statsPanel);

    // Спарклайн
    m_sparkline = new SparklineWidget(this);
    mainLayout->addWidget(m_sparkline);

    // Разделитель
    m_separator = new QFrame(this);
    m_separator->setFrameShape(QFrame::HLine);
    m_separator->setFrameShadow(QFrame::Sunken);
    m_separator->setStyleSheet(separatorStyle());
    mainLayout->addWidget(m_separator);

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

    // Кнопка Выход
    m_exitButton = new QPushButton(tr("Выход"), this);
    connect(m_exitButton, &QPushButton::clicked, this, [this]() {
        emit exitRequested();
    });
    mainLayout->addWidget(m_exitButton);
}

void TrayMenu::applyPlatformStyle() {
    if (m_applyingPlatformStyle) {
        return;
    }

    QScopedValueRollback<bool> applyingGuard(m_applyingPlatformStyle, true);

#ifdef Q_OS_MAC
    const bool dark = isDarkPalette(QApplication::palette());
    const QString text = dark
        ? QStringLiteral("#f5f5f7")
        : QStringLiteral("#1d1d1f");
    const QString hover = dark
        ? QStringLiteral("rgba(10, 132, 255, 58)")
        : QStringLiteral("rgba(0, 122, 255, 32)");
    const QString pressed = dark
        ? QStringLiteral("rgba(10, 132, 255, 82)")
        : QStringLiteral("rgba(0, 122, 255, 50)");

    const QString platformStyle = QString(R"(
        TrayMenu {
            background-color: transparent;
            border: none;
            font-size: 10.5pt;
        }
        QLabel {
            color: %1;
            padding: 1px 0;
            font-size: 10.5pt;
        }
        QPushButton {
            color: %1;
            background-color: transparent;
            border: none;
            border-radius: 7px;
            min-height: 24px;
            padding: 5px 10px;
            text-align: left;
            font-size: 10.5pt;
        }
        QPushButton:hover {
            background-color: %2;
        }
        QPushButton:pressed {
            background-color: %3;
        }
    )").arg(text, hover, pressed);
#else
    const QString platformStyle = QStringLiteral(R"(
        TrayMenu {
            background-color: #2b2b2b;
            border: 1px solid #555;
            border-radius: 6px;
            font-size: 8.25pt;
        }
        QLabel {
            color: #ddd;
            padding: 2px 0;
            font-size: 8.25pt;
        }
        QPushButton {
            color: #ddd;
            background-color: transparent;
            border: none;
            border-radius: 4px;
            padding: 6px 16px;
            text-align: left;
            font-size: 8.25pt;
        }
        QPushButton:hover {
            background-color: #3a3a3a;
        }
        QPushButton:pressed {
            background-color: #444;
        }
    )");
#endif

    if (m_appliedPlatformStyle != platformStyle) {
        m_appliedPlatformStyle = platformStyle;
        setStyleSheet(platformStyle);
    }

    if (m_separator) {
        m_separator->setStyleSheet(separatorStyle());
    }
}

void TrayMenu::setStats(uint64_t uptimeSecs, uint64_t websocketActive, uint64_t websocketPeak,
                        uint64_t ipRotations, uint64_t ipSucRotations, 
                        double peakRx, double peakTx,
                        uint64_t totalRx, uint64_t totalTx) {
    if (m_statsPanel)
        m_statsPanel->updateStats(uptimeSecs, websocketActive, websocketPeak, 
            ipRotations, ipSucRotations , peakRx, peakTx, totalRx, totalTx);
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
#ifdef Q_OS_MAC
    MacPlatformIntegration::closePopover();
#else
    hide();
#endif
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
            hideMenu();
        }
    } else {
        if (!underMouse() && !isFocused) {
            hideMenu();
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
