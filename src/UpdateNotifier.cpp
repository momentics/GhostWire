#include "UpdateNotifier.h"
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

UpdateNotifier::UpdateNotifier(QSystemTrayIcon* trayIcon, QObject* parent)
    : QObject(parent)
    , m_trayIcon(trayIcon)
{
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

bool UpdateNotifier::notifyUpdateAvailableAuto(const QString& currentVersion,
                                                const QString& latestVersion,
                                                const QString& releaseUrl) {
#ifdef Q_OS_WIN
    // Windows: non-blocking tray notification for auto checks
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
#else
    // macOS/Linux: blocking dialog (same for auto and manual)
    return showUpdateAvailableDialog(currentVersion, latestVersion, releaseUrl);
#endif
}

bool UpdateNotifier::notifyUpdateAvailableManual(const QString& currentVersion,
                                                  const QString& latestVersion,
                                                  const QString& releaseUrl) {
#ifdef Q_OS_WIN
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
#else
    // macOS/Linux: same dialog as auto
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
#ifdef Q_OS_WIN
    if (m_trayIcon) {
        m_trayIcon->showMessage(
            tr("Невозможно выполнить команду"),
            tr("Ресурсы для запуска отсутствуют"),
            QSystemTrayIcon::Warning,
            5000
        );
    }
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
    QMessageBox::information(nullptr,
        tr("Обновления"),
        tr("Вы используете последнюю версию."));
}

void UpdateNotifier::showCheckFailedWarning(const QString& error) {
    QMessageBox::warning(nullptr,
        tr("Проверка обновлений"),
        tr("Не удалось проверить обновления: %1").arg(error));
}
