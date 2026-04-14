#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "Application.h"
#include "GhostWire.h"
#include "TrayManager.h"
#include "TrayMenu.h"
#include "UpdateChecker.h"
#include "Config.h"
#include "Utils.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QCursor>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QMessageBox>

Application::Application(QObject* parent)
    : QObject(parent)
{
    m_ghostWire   = std::make_unique<GhostWire>();
    m_trayManager = std::make_unique<TrayManager>();
    m_trayMenu    = std::make_unique<TrayMenu>();
    m_updateChecker = std::make_unique<UpdateChecker>(
        QCoreApplication::applicationVersion(),
        Config::GITHUB_REPO_OWNER,
        Config::GITHUB_REPO_NAME);

    // Соединяем сигналы TrayMenu к Application
    connect(m_trayMenu.get(), &TrayMenu::startRequested, this, [this]() {
        if (m_ghostWire->start()) {
            m_proxyRunning = true;
            m_trayManager->setState(true);
            m_trayMenu->setRunningState(true);
            m_trayMenu->clearSparkline();
            m_hasPrevStats = false;
            m_prevWsActive = 0; // Сбросить — первый тик статистики установит корректное состояние
            if (m_statsTimer) m_statsTimer->start(Config::STATS_POLL_INTERVAL_MS);
        }
    });

    connect(m_trayMenu.get(), &TrayMenu::stopRequested, this, [this]() {
        // Сначала останавливаем таймер
        if (m_statsTimer) m_statsTimer->stop();
        m_proxyRunning = false;
        m_ghostWire->stop();
        // Мгновенно очищаем UI
        m_trayManager->setState(false);
        m_trayManager->setConnectionsState(false);
        m_trayMenu->setRunningState(false);
        m_trayMenu->clearSparkline();
        m_trayMenu->setStats(0, 0, 0, 0, 0, 0);
        m_hasPrevStats = false;
        m_peakRx = 0;
        m_peakTx = 0;
        m_prevWsActive = 0;
        m_prevBytesReceived = 0;
        m_prevBytesSent = 0;
    });

    connect(m_trayMenu.get(), &TrayMenu::exitRequested, this, &Application::onTrayExit);

    connect(m_trayMenu.get(), &TrayMenu::configureTelegramRequested,
            this, &Application::onConfigureTelegram);

    // Соединяем сигналы TrayManager к Application (для открытия меню)
    connect(m_trayManager.get(), &TrayManager::iconClicked, this, [this](const QRect& iconRect) {
        showTrayMenu(iconRect);
#ifdef Q_OS_LINUX
        if (m_trayMenu) m_trayMenu->startIpcFocusMonitor(true);
#endif
    });

#ifdef Q_OS_LINUX
    connect(m_trayManager.get(), &TrayManager::linuxQuitRequested, this, &Application::onTrayExit);
#endif

    connect(m_trayMenu.get(), &TrayMenu::checkUpdatesRequested,
            this, &Application::onCheckUpdatesRequested);

    connect(m_updateChecker.get(), &UpdateChecker::updateAvailable,
            this, &Application::onUpdateAvailable);
    connect(m_updateChecker.get(), &UpdateChecker::noUpdate,
            this, &Application::onNoUpdate);
    connect(m_updateChecker.get(), &UpdateChecker::checkFailed,
            this, &Application::onUpdateCheckFailed);
}

Application::~Application() = default;

bool Application::initialize() {
    // 1. Загрузить библиотеку
    if (!m_ghostWire->load()) {
        qCritical() << "Application: не удалось загрузить ghostwire.dll";
        return false;
    }

    // 2. Загрузить конфиг из ресурсов
    QString config = loadConfig();
    if (config.isEmpty()) {
        qCritical() << "Application: не удалось загрузить конфигурацию";
        return false;
    }

    // 3. Создать экземпляр прокси
    if (!m_ghostWire->create(config)) {
        qCritical() << "Application: не удалось создать экземпляр прокси";
        return false;
    }

    // 4. Инициализировать трей
    m_trayManager->init();
    m_trayMenu->setRunningState(false);

    // 5. Создать таймер опроса статистики (запускается при Start)
    m_statsTimer = std::make_unique<QTimer>(this);
    connect(m_statsTimer.get(), &QTimer::timeout, this, &Application::onStatsTick);

    // 6. Восстановить предыдущее состояние
    restoreState();

    // 7. Запустить проверку обновлений
    m_updateChecker->checkForUpdates();

    qDebug() << "Application: инициализация завершена";
    return true;
}

QString Application::loadConfig() const {
    QFile file(Config::LIB_CONFIG_RESOURCE);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Application: не удалось открыть" << Config::LIB_CONFIG_RESOURCE;
        return QString();
    }
    QTextStream stream(&file);
    return stream.readAll();
}

void Application::restoreState() {
    QSettings settings("GhostWire", "GhostWireDesktop");
    bool wasRunning = settings.value("proxyRunning", false).toBool();

    if (wasRunning) {
        // Восстановить предыдущее состояние — запустить прокси
        qDebug() << "Application: восстановление предыдущего состояния — запуск прокси";
        if (m_ghostWire->start()) {
            m_proxyRunning = true;
            m_trayManager->setState(true);
            m_trayMenu->setRunningState(true);
            m_trayMenu->clearSparkline();
            m_hasPrevStats = false;
            m_prevWsActive = 0;
            m_statsTimer->start(Config::STATS_POLL_INTERVAL_MS);
        }
    }
    // Если wasRunning == false — остаёмся в состоянии STOP (по умолчанию)
}

void Application::saveState() {
    QSettings settings("GhostWire", "GhostWireDesktop");
    settings.setValue("proxyRunning", m_proxyRunning);
    settings.sync();
    qDebug() << "Application: сохранено состояние proxyRunning =" << m_proxyRunning;
}

void Application::onStatsTick() {
    if (!m_ghostWire || !m_proxyRunning) return;

    auto stats = m_ghostWire->getStats();

    // Отслеживаем изменение количества WS-соединений для индикации
    if (stats.websocket_active != m_prevWsActive) {
        m_prevWsActive = stats.websocket_active;
        m_trayManager->setConnectionsState(stats.websocket_active > 0);
    }

    // Рассчитать дельту RX/TX и обновить пики
    double rxDelta = 0;
    double txDelta = 0;
    if (m_hasPrevStats) {
        // Защита от wrap-around: если счётчик сбросился, дельта = 0
        if (stats.bytes_received >= m_prevBytesReceived)
            rxDelta = static_cast<double>(stats.bytes_received - m_prevBytesReceived);
        if (stats.bytes_sent >= m_prevBytesSent)
            txDelta = static_cast<double>(stats.bytes_sent - m_prevBytesSent);

        // Нормализуем в байты/сек для пика
        double intervalSec = Config::STATS_POLL_INTERVAL_MS / 1000.0;
        double rxRate = rxDelta / intervalSec;
        double txRate = txDelta / intervalSec;
        if (rxRate > m_peakRx) m_peakRx = rxRate;
        if (txRate > m_peakTx) m_peakTx = txRate;

        m_trayMenu->addSparklinePoint(rxRate, txRate);
    }

    // Обновить UI
    m_trayMenu->setStats(stats.uptime_secs, stats.websocket_active,
                         m_peakRx, m_peakTx,
                         stats.bytes_received, stats.bytes_sent);
    m_trayMenu->setRunningState(true);

    m_prevBytesReceived = stats.bytes_received;
    m_prevBytesSent = stats.bytes_sent;
    m_hasPrevStats = true;
}

void Application::onTrayExit() {
    // Сохранить состояние перед выходом
    saveState();

    // Останавливаем и отключаем таймер
    if (m_statsTimer) {
        m_statsTimer->stop();
    }
    m_proxyRunning = false;
    m_trayManager->cleanup();
    if (m_ghostWire) {
        m_ghostWire->stop();
        m_ghostWire->destroy();
    }
    qApp->quit();
}

void Application::showTrayMenuAtCursor() {
    if (!m_trayMenu) return;

#ifdef Q_OS_LINUX
    // Linux: геометрия трея недоступна — используем определение положения панели
    showTrayMenu(QRect());
    m_trayMenu->startIpcFocusMonitor(true, true);
#else
    // Windows: пытаемся получить реальную позицию иконки трея
    QRect iconRect = m_trayManager->trayIconGeometry();
    if (iconRect.isValid() && !iconRect.isEmpty()) {
        showTrayMenu(iconRect);
        m_trayMenu->startIpcFocusMonitor(true, true);
        qDebug() << "Application: tray menu shown at tray icon geometry" << iconRect;
    } else {
        // Fallback к позиции курсора
        m_trayMenu->adjustSize();
        QPoint cursorPos = QCursor::pos();
        showTrayMenuAtPoint(cursorPos);
        m_trayMenu->startIpcFocusMonitor(true, true);
        qDebug() << "Application: tray menu shown at cursor (geometry unavailable)";
    }
#endif
}

void Application::showTrayMenu(const QRect& iconRect) {
    if (!m_trayMenu) return;

    m_trayMenu->adjustSize();

#ifdef Q_OS_LINUX
    // Linux: TrayManager передаёт пустой QRect — используем определение положения панели
    if (iconRect.isEmpty()) {
        QPoint pos = calculateLinuxTrayMenuPosition(m_trayMenu->width(), m_trayMenu->height());
        showTrayMenuAtPoint(pos);
        qDebug() << "Application: tray menu shown at Linux panel-detected position" << pos;
        return;
    }
#endif

    // Windows: используем реальную геометрию иконки трея
    showTrayMenuAtPoint(iconRect.topLeft());
    qDebug() << "Application: tray menu shown at" << iconRect.topLeft();
}

void Application::showTrayMenuAtPoint(const QPoint& pos) {
    if (!m_trayMenu) return;

    int x = pos.x();
    int y = pos.y() - m_trayMenu->height();

    QScreen* screen = QGuiApplication::screenAt(pos);
    if (!screen) screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();

    // Если меню не помещается над точкой — показываем под ней
    const int menuOffsetBelow = 4;
    if (y < screenRect.top()) {
        y = pos.y() + menuOffsetBelow;
    }

    // Горизонтальная коррекция
    if (x + m_trayMenu->width() > screenRect.right())
        x = screenRect.right() - m_trayMenu->width();
    if (x < screenRect.left()) x = screenRect.left();

    // Порядок показа критичен для Windows 11:
    // 1. raise() — поднимаем наверх z-order ДО показа
    // 2. show()   — показываем окно
    // 3. activateWindow() — передаём фокус (для Qt::Popup работает на всех платформах)
    m_trayMenu->move(x, y);
    m_trayMenu->raise();
    m_trayMenu->show();
    m_trayMenu->activateWindow();
}

/// Проверить, запущен ли процесс Telegram Desktop (кроссплатформенно)
static bool isTelegramRunning() {
    QProcess proc;

#ifdef _WIN32
    proc.start("tasklist", QStringList() << "/FI"
        << QString("IMAGENAME eq %1").arg(Config::TELEGRAM_PROCESS_NAME)
        << "/NH");
#elif defined(__APPLE__)
    proc.start("pgrep", QStringList() << "-i" << Config::TELEGRAM_PROCESS_NAME);
#else
    // Linux: pgrep с case-insensitive по частичному имени «telegram»
    // ловит telegram-desktop, telegram-desktop-bin и т.д.
    proc.start("pgrep", QStringList() << "-i" << "telegram");
#endif

    proc.waitForFinished(3000);
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

#ifdef _WIN32
    return output.contains(Config::TELEGRAM_PROCESS_NAME, Qt::CaseInsensitive);
#else
    // pgrep возвращает PID (число) если процесс найден, пусто если нет
    return !output.trimmed().isEmpty();
#endif
}

/// Проверить, зарегистрирован ли обработчик tg:// протокола
bool Application::isTelegramSchemeRegistered() const {
#ifdef _WIN32
    // Проверяем реестр: HKEY_CLASSES_ROOT/tg/
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"tg", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
#elif defined(__APPLE__)
    // macOS: через LaunchServices проверяем, есть ли приложение для tg://
    QProcess proc;
    proc.start("osascript", QStringList()
        << "-e" << "on run {url}"
        << "-e" << "tell application \"System Events\" to get (name of processes whose name is \"Telegram\")"
        << "-e" << "end run"
        << "tg://");
    proc.waitForFinished(2000);
    // Если Telegram запущен — handler точно есть
    if (proc.exitCode() == 0 && !proc.readAllStandardOutput().trimmed().isEmpty())
        return true;
    // Иначе проверяем через launchctl: есть ли приложение, обрабатывающее tg://
    QProcess lsProc;
    lsProc.start("sh", QStringList() << "-c"
        << "lsregister -dump 2>/dev/null | grep -qi 'tg:' && echo yes");
    lsProc.waitForFinished(2000);
    return lsProc.readAllStandardOutput().trimmed() == "yes";
#else
    // Linux: через xdg-settings проверяем handler для tg://
    QProcess proc;
    proc.start("xdg-settings", QStringList()
        << "get" << "default-url-scheme-handler" << "tg");
    proc.waitForFinished(2000);
    QString handler = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    return !handler.isEmpty() && !handler.contains("not found", Qt::CaseInsensitive);
#endif
}

void Application::onConfigureTelegram() {
    // Скрываем меню в любом случае
    m_trayMenu->hideMenu();

    // Проверяем, зарегистрирован ли tg:// handler
    if (!isTelegramSchemeRegistered()) {
        m_trayManager->showMessage(
            tr("Telegram не установлен"),
            tr("Установите Telegram Desktop для автоматической настройки прокси"),
            QSystemTrayIcon::Warning,
            3000
        );
        qDebug() << "Application: tg:// handler не зарегистрирован";
        return;
    }

    // Проверяем, запущен ли процесс Telegram
    if (isTelegramRunning()) {
        // Telegram запущен — открываем моникер
        QString url = QString("tg://socks?server=%1&port=%2")
            .arg(Config::SOCKS_SERVER)
            .arg(Config::SOCKS_PORT);
        QDesktopServices::openUrl(QUrl(url));
        qDebug() << "Application: отправлен моникер для подключения Telegram к прокси";
    } else {
        // Telegram не запущен — уведомляем пользователя
        m_trayManager->showMessage(
            tr("Telegram не запущен"),
            tr("Сначала запустите Telegram Desktop"),
            QSystemTrayIcon::Warning,
            3000
        );
        qDebug() << "Application: Telegram Desktop не запущен";
    }
}

void Application::onCheckUpdatesRequested() {
    qDebug() << "Application: ручная проверка обновлений";
    m_trayMenu->hideMenu();
    m_isManualUpdateCheck = true;
    m_updateChecker->checkForUpdatesNow();
}

void Application::onUpdateAvailable(const QString& version, const QString& releaseUrl) {
    qDebug() << "Application: доступна версия" << version;
    QString current = QCoreApplication::applicationVersion();
    if (m_isManualUpdateCheck) {
        m_isManualUpdateCheck = false;
        if (QMessageBox::question(nullptr, tr("Доступна новая версия"),
                                  QString(tr("Текущая: %1, новая: %2. Открыть страницу загрузки?")).arg(current, version),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl(releaseUrl));
        }
    } else {
        m_trayManager->showMessage(
            tr("Доступна новая версия"),
            QString(tr("Текущая: %1, новая: %2.")).arg(current, version),
            QSystemTrayIcon::Information,
            10000
        );
    }
}

void Application::onNoUpdate() {
    qDebug() << "Application: обновлений нет";
    if (m_isManualUpdateCheck) {
        m_isManualUpdateCheck = false;
        QMessageBox::information(nullptr, tr("Обновления"), tr("Вы используете последнюю версию."));
    }
}

void Application::onUpdateCheckFailed(const QString& error) {
    qDebug() << "Application: проверка обновлений не удалась —" << error;
    if (m_isManualUpdateCheck) {
        m_isManualUpdateCheck = false;
        QMessageBox::warning(nullptr, tr("Проверка обновлений"), tr("Не удалось проверить обновления: %1").arg(error));
    }
}
