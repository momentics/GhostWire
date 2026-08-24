#include "UpdateNotifier.h"
#ifdef Q_OS_MAC
#include "MacPlatformIntegration.h"
#endif
#include <QSystemTrayIcon>
#include <QMessageBox>

UpdateNotifier::UpdateNotifier(QSystemTrayIcon* trayIcon, QObject* parent)
    : QObject(parent)
    , m_trayIcon(trayIcon)
{
#ifdef Q_OS_MAC
    MacPlatformIntegration::setNotificationActionHandler([this](const QString& releaseUrl) {
        if (!releaseUrl.isEmpty()) {
            emit openReleaseUrl(releaseUrl);
        }
    });
#endif

    if (m_trayIcon) {
        connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, [this]() {
            if (m_pendingReleaseUrl.isEmpty())
                return;

            const QString releaseUrl = m_pendingReleaseUrl;
            m_pendingReleaseUrl.clear();
            emit openReleaseUrl(releaseUrl);
        });
    }
}

UpdateNotifier::~UpdateNotifier() {
#ifdef Q_OS_MAC
    MacPlatformIntegration::setNotificationActionHandler(nullptr);
#endif
}

bool UpdateNotifier::notifyUpdateAvailableAuto(const QString& currentVersion,
                                                const QString& latestVersion,
                                                const QString& releaseUrl) {
#if defined(Q_OS_WIN)
    // Windows: неблокирующее уведомление трея для автоматических проверок
    m_pendingReleaseUrl = releaseUrl;
    if (m_trayIcon) {
        m_trayIcon->showMessage(
            tr("Доступна новая версия"),
            QString(tr("Текущая: %1, новая: %2")).arg(currentVersion, latestVersion),
            QSystemTrayIcon::Information,
            10000
        );
    } else {
        m_pendingReleaseUrl.clear();
    }
    return false;
#elif defined(Q_OS_MAC)
    MacPlatformIntegration::showNotification(
        tr("Доступна новая версия"),
        QString(tr("Текущая: %1, новая: %2")).arg(currentVersion, latestVersion),
        releaseUrl);
    return false;
#else
    // Linux: blocking dialog, because tray notification support varies significantly by desktop.
    return showUpdateAvailableDialog(currentVersion, latestVersion, releaseUrl);
#endif
}

bool UpdateNotifier::notifyUpdateAvailableManual(const QString& currentVersion,
                                                  const QString& latestVersion,
                                                  const QString& releaseUrl) {
#if defined(Q_OS_WIN)
    m_pendingReleaseUrl.clear();
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Доступна новая версия"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(tr("Текущая версия: %1\nНовая версия: %2").arg(currentVersion, latestVersion));
    msgBox.setInformativeText(tr("Открыть страницу загрузки?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        emit openReleaseUrl(releaseUrl);
        return true;
    }
    return false;
#elif defined(Q_OS_MAC)
    const bool openRelease = MacPlatformIntegration::showConfirmationAlert(
        tr("Доступна новая версия"),
        tr("Текущая версия: %1\nНовая версия: %2").arg(currentVersion, latestVersion),
        tr("Открыть страницу загрузки?"),
        tr("Открыть"),
        tr("Отмена"));
    if (openRelease) {
        emit openReleaseUrl(releaseUrl);
        return true;
    }
    return false;
#else
    return showUpdateAvailableDialog(currentVersion, latestVersion, releaseUrl);
#endif
}

void UpdateNotifier::notifyNoUpdateManual() {
    m_pendingReleaseUrl.clear();
    showNoUpdateInfo();
}

void UpdateNotifier::notifyCheckFailedManual(const QString& error) {
    m_pendingReleaseUrl.clear();
    showCheckFailedWarning(error);
}

void UpdateNotifier::notifyStartupResourcesUnavailable() {
    m_pendingReleaseUrl.clear();
#if defined(Q_OS_WIN)
    if (m_trayIcon) {
        m_trayIcon->showMessage(
            tr("Невозможно выполнить команду"),
            tr("Ресурсы для запуска отсутствуют"),
            QSystemTrayIcon::Warning,
            5000
        );
    }
#elif defined(Q_OS_MAC)
    MacPlatformIntegration::showNotification(
        tr("Невозможно выполнить команду"),
        tr("Ресурсы для запуска отсутствуют"));
#else
    QMessageBox::warning(nullptr,
        tr("Невозможно выполнить команду"),
        tr("Ресурсы для запуска отсутствуют"));
#endif
}

bool UpdateNotifier::showUpdateAvailableDialog(const QString& currentVersion,
                                                const QString& latestVersion,
                                                const QString& releaseUrl) {
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Доступна новая версия"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(tr("Текущая версия: %1\nНовая версия: %2").arg(currentVersion, latestVersion));
    msgBox.setInformativeText(tr("Открыть страницу загрузки?"));
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Open);

    if (msgBox.exec() == QMessageBox::Open) {
        emit openReleaseUrl(releaseUrl);
        return true;
    }
    return false;
}

void UpdateNotifier::showNoUpdateInfo() {
#ifdef Q_OS_MAC
    MacPlatformIntegration::showInfoAlert(
        tr("Обновления"),
        tr("Вы используете последнюю версию."),
        tr("OK"));
#else
    QMessageBox::information(nullptr,
        tr("Обновления"),
        tr("Вы используете последнюю версию."));
#endif
}

void UpdateNotifier::showCheckFailedWarning(const QString& error) {
#ifdef Q_OS_MAC
    MacPlatformIntegration::showWarningAlert(
        tr("Проверка обновлений"),
        tr("Не удалось проверить обновления: %1").arg(error),
        tr("OK"));
#else
    QMessageBox::warning(nullptr,
        tr("Проверка обновлений"),
        tr("Не удалось проверить обновления: %1").arg(error));
#endif
}
