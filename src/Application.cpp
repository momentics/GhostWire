#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "Application.h"
#include "GhostWire.h"
#include "TrayManager.h"
#include "IGhostWire.h"
#include "ITrayManager.h"
#include "TrayMenu.h"
#include "UpdateChecker.h"
#include "UpdateNotifier.h"
#include "StatsTracker.h"
#include "SettingsManager.h"
#include "Config.h"
#include "Utils.h"
#include "config_bin.h"

#include <QJsonParseError>
#include <QJsonDocument>
#include <QDebug>
#include <QCursor>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QMessageBox>

namespace {
constexpr int updateCheckIntervalMs() {
    return Config::UPDATE_CHECK_INTERVAL_HOURS * 60 * 60 * 1000;
}
}

Application::Application(QObject* parent)
    : QObject(parent)
{
    m_ghostWire   = std::make_unique<GhostWire>();
    m_trayManager = std::make_unique<TrayManager>();
    m_trayMenu    = std::make_unique<TrayMenu>();
    m_statsTracker = std::make_unique<StatsTracker>();
    m_settings     = std::make_unique<SettingsManager>();
    m_updateChecker = std::make_unique<UpdateChecker>(
        QCoreApplication::applicationVersion(),
        Config::GITHUB_REPO_OWNER,
        Config::GITHUB_REPO_NAME);
    m_updateNotifier = std::make_unique<UpdateNotifier>(m_trayManager->trayIcon(), this);

    // Соединяем сигналы TrayMenu к Application
    connect(m_trayMenu.get(), &TrayMenu::startRequested, this, [this]() {
        startProxy();
    });

    connect(m_trayMenu.get(), &TrayMenu::stopRequested, this, [this]() {
        // Сначала останавливаем таймер
        if (m_statsTimer) m_statsTimer->stop();
        m_proxyRunning = false;
        m_ghostWire->stop();
        // Мгновенно очищаем UI
        m_trayManager->setState(GHOSTWIRE_PROXY_OFFLINE);
        m_trayManager->setConnectionsState(false);
        m_trayMenu->setRunningState(false);
        m_trayMenu->clearSparkline();
        m_trayMenu->setStats(0, 0, 0, 0, 0, 0, 0, 0, 0);
        m_statsTracker->reset();
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

    // UpdateNotifier -> открыть URL
    connect(m_updateNotifier.get(), &UpdateNotifier::openReleaseUrl,
            this, &Application::onOpenReleaseUrl);
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
    m_trayManager->setState(GHOSTWIRE_PROXY_OFFLINE);
    m_trayMenu->setRunningState(false);

    // 5. Создать таймер опроса статистики (запускается при Start)
    m_statsTimer = std::make_unique<QTimer>(this);
    connect(m_statsTimer.get(), &QTimer::timeout, this, &Application::onStatsTick);

    // 6. Создать таймер периодической проверки обновлений.
    m_updateCheckTimer = std::make_unique<QTimer>(this);
    m_updateCheckTimer->setSingleShot(true);
    connect(m_updateCheckTimer.get(), &QTimer::timeout, this, [this]() {
        qDebug() << "Application: периодическая проверка обновлений";
        m_updateChecker->checkForUpdates();
        scheduleNextUpdateCheck();
    });

    // 7. Восстановить предыдущее состояние
    restoreState();

    // 8. Запустить проверку обновлений и последующие периодические проверки.
    m_updateChecker->checkForUpdatesOnStartup();
    scheduleNextUpdateCheck();

    qDebug() << "Application: инициализация завершена";
    return true;
}

QString Application::loadConfig() const {
    if constexpr (g_config_key_len == 0 || g_config_size == 0) {
        return QString();
    } else {
        QByteArray result(static_cast<int>(g_config_size), 0);
        for (size_t i = 0; i < g_config_size; i++) {
            const unsigned char keyByte = g_config_key[i % g_config_key_len];
            result[static_cast<int>(i)] = static_cast<char>(g_config_data[i] ^ keyByte);
        }

        // Валидация JSON
        QJsonParseError parseError;
        QJsonDocument::fromJson(result, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            return QString();
        }

        return QString::fromUtf8(result);
    }
}

void Application::restoreState() {
    bool wasRunning = m_settings->getProxyRunning();

    if (wasRunning) {
        // Восстановить предыдущее состояние — запустить прокси
        qDebug() << "Application: восстановление предыдущего состояния — запуск прокси";
        startProxy();
    }
    // Если wasRunning == false — остаёмся в состоянии STOP (по умолчанию)
}

bool Application::startProxy() {
    if (m_ghostWire->start()) {
        m_proxyRunning = true;
        m_trayManager->setState(m_ghostWire->state());
        m_trayManager->setConnectionsState(false);
        m_trayMenu->setRunningState(true);
        m_trayMenu->clearSparkline();
        m_statsTracker->reset();
        if (m_statsTimer) m_statsTimer->start(Config::STATS_POLL_INTERVAL_MS);
        return true;
    }

    m_proxyRunning = false;
    if (m_statsTimer) m_statsTimer->stop();
    m_trayManager->setState(GHOSTWIRE_PROXY_OFFLINE);
    m_trayManager->setConnectionsState(false);
    m_trayMenu->setRunningState(false);
    m_updateNotifier->notifyStartupResourcesUnavailable();
    qWarning() << "Application: запуск прокси не выполнен — ресурсы для запуска отсутствуют";
    return false;
}

void Application::saveState() {
    m_settings->setProxyRunning(m_proxyRunning);
    qDebug() << "Application: сохранено состояние proxyRunning =" << m_proxyRunning;
}

void Application::scheduleNextUpdateCheck() {
    if (!m_updateCheckTimer || !m_updateChecker)
        return;

    const int nextCheckDelayMs = m_updateChecker->millisecondsUntilNextCheck();
    m_updateCheckTimer->start(nextCheckDelayMs > 0 ? nextCheckDelayMs : updateCheckIntervalMs());
}

void Application::onStatsTick() {
    if (!m_ghostWire || !m_proxyRunning) return;

    const auto proxyState = m_ghostWire->state();
    if (proxyState == GHOSTWIRE_PROXY_OFFLINE) {
        m_proxyRunning = false;
        if (m_statsTimer) m_statsTimer->stop();
        m_trayManager->setState(GHOSTWIRE_PROXY_OFFLINE);
        m_trayManager->setConnectionsState(false);
        m_trayMenu->setRunningState(false);
        return;
    }

    m_trayManager->setState(proxyState);

    auto stats = m_ghostWire->getStats();

    double peakRx = 0;
    double peakTx = 0;
    double rxRate = 0;
    double txRate = 0;
    bool hasConnections = false;

    if (!m_statsTracker->processTick(stats, proxyState, peakRx, peakTx, rxRate, txRate, hasConnections)) {
        m_proxyRunning = false;
        if (m_statsTimer) m_statsTimer->stop();
        m_trayManager->setState(GHOSTWIRE_PROXY_OFFLINE);
        m_trayManager->setConnectionsState(false);
        m_trayMenu->setRunningState(false);
        return;
    }

    m_trayManager->setConnectionsState(
        proxyState == GHOSTWIRE_PROXY_ONLINE && stats.websocket_active > 0);

    m_trayMenu->addSparklinePoint(rxRate, txRate);

    m_trayMenu->setStats(stats.uptime_secs, stats.websocket_active, stats.peak_active_connections,
                          stats.ip_rotations, stats.rotation_success,
                          peakRx, peakTx, stats.bytes_received, stats.bytes_sent);

    m_trayMenu->setRunningState(true);
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
    m_updateChecker->checkForUpdatesNow();
}

void Application::onUpdateAvailable(const QString& version, const QString& releaseUrl, bool manual) {
    qDebug() << "Application: доступна версия" << version;

    bool openedReleaseUrl = false;
    if (manual) {
        // Ручная проверка — интерактивное подтверждение
        openedReleaseUrl = m_updateNotifier->notifyUpdateAvailableManual(
            QCoreApplication::applicationVersion(), version, releaseUrl);
    } else {
        // Автоматическая проверка — только уведомление (toast на Windows, MessageBox на Linux/macOS)
        openedReleaseUrl = m_updateNotifier->notifyUpdateAvailableAuto(
            QCoreApplication::applicationVersion(), version, releaseUrl);
    }

    if (!openedReleaseUrl) {
        m_updateChecker->postponeUpdateNotification();
    } else {
        m_updateChecker->clearUpdateNotificationPostponement();
    }
    scheduleNextUpdateCheck();
}

void Application::onNoUpdate(bool manual) {
    qDebug() << "Application: обновлений нет";
    if (manual) {
        m_updateNotifier->notifyNoUpdateManual();
    }
    // Автоматический режим: ничего не делаем
}

void Application::onUpdateCheckFailed(const QString& error, bool manual) {
    qDebug() << "Application: проверка обновлений не удалась —" << error;
    if (manual) {
        m_updateNotifier->notifyCheckFailedManual(error);
    }
    // Автоматический режим: ничего не делаем
}

void Application::onOpenReleaseUrl(const QString& url) {
    qDebug() << "Application: открытие страницы обновления:" << url;
    if (m_updateChecker) {
        m_updateChecker->clearUpdateNotificationPostponement();
    }
    QDesktopServices::openUrl(QUrl(url));
}
