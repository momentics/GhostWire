#include "UpdateNotifier.h"
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

UpdateNotifier::UpdateNotifier(QSystemTrayIcon* trayIcon, QObject* parent)
    : QObject(parent)
    , m_trayIcon(trayIcon)
{
}

void UpdateNotifier::notifyUpdateAvailableAuto(const QString& currentVersion,
                                                const QString& latestVersion,
                                                const QString& releaseUrl) {
#ifdef Q_OS_WIN
    notifyUpdateAvailableAutoWindows(currentVersion, latestVersion, releaseUrl);
#elif defined(Q_OS_MACOS)
    notifyUpdateAvailableAutoMacOS(currentVersion, latestVersion, releaseUrl);
#else
    notifyUpdateAvailableAutoLinux(currentVersion, latestVersion, releaseUrl);
#endif
}

void UpdateNotifier::notifyUpdateAvailableManual(const QString& currentVersion,
                                                  const QString& latestVersion,
                                                  const QString& releaseUrl) {
#ifdef Q_OS_WIN
    notifyUpdateAvailableManualWindows(currentVersion, latestVersion, releaseUrl);
#elif defined(Q_OS_MACOS)
    notifyUpdateAvailableManualMacOS(currentVersion, latestVersion, releaseUrl);
#else
    notifyUpdateAvailableManualLinux(currentVersion, latestVersion, releaseUrl);
#endif
}

void UpdateNotifier::notifyNoUpdateManual() {
#ifdef Q_OS_WIN
    notifyNoUpdateManualWindows();
#elif defined(Q_OS_MACOS)
    notifyNoUpdateManualMacOS();
#else
    notifyNoUpdateManualLinux();
#endif
}

void UpdateNotifier::notifyCheckFailedManual(const QString& error) {
#ifdef Q_OS_WIN
    notifyCheckFailedManualWindows(error);
#elif defined(Q_OS_MACOS)
    notifyCheckFailedManualMacOS(error);
#else
    notifyCheckFailedManualLinux(error);
#endif
}

void UpdateNotifier::notifyStartupResourcesUnavailable() {
#ifdef Q_OS_WIN
    notifyStartupResourcesUnavailableWindows();
#elif defined(Q_OS_MACOS)
    notifyStartupResourcesUnavailableMacOS();
#else
    notifyStartupResourcesUnavailableLinux();
#endif
}

#ifdef Q_OS_WIN

void UpdateNotifier::notifyUpdateAvailableAutoWindows(const QString& currentVersion,
                                                       const QString& latestVersion,
                                                       const QString& /*releaseUrl*/) {
    // На Windows авто-уведомление — только toast, без блокирующих диалогов
    if (m_trayIcon) {
        m_trayIcon->showMessage(
            tr("Доступна новая версия"),
            QString(tr("Текущая: %1, новая: %2")).arg(currentVersion, latestVersion),
            QSystemTrayIcon::Information,
            10000
        );
    }
}

void UpdateNotifier::notifyUpdateAvailableManualWindows(const QString& currentVersion,
                                                         const QString& latestVersion,
                                                         const QString& releaseUrl) {
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Доступна новая версия"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(tr("Текущая версия: %1\nНовая версия: %2").arg(currentVersion, latestVersion));
    msgBox.setInformativeText(tr("Открыть страницу загрузки?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        emit openReleaseUrl(releaseUrl);
    }
}

void UpdateNotifier::notifyNoUpdateManualWindows() {
    QMessageBox::information(nullptr,
        tr("Обновления"),
        tr("Вы используете последнюю версию."));
}

void UpdateNotifier::notifyCheckFailedManualWindows(const QString& error) {
    QMessageBox::warning(nullptr,
        tr("Проверка обновлений"),
        tr("Не удалось проверить обновления: %1").arg(error));
}

void UpdateNotifier::notifyStartupResourcesUnavailableWindows() {
    if (m_trayIcon) {
        m_trayIcon->showMessage(
            tr("Невозможно выполнить команду"),
            tr("Ресурсы для запуска отсутствуют"),
            QSystemTrayIcon::Warning,
            5000
        );
    }
}

#endif

#ifdef Q_OS_MACOS

// macOS: пока дублирует Linux — уточним поведение после тестирования
void UpdateNotifier::notifyUpdateAvailableAutoMacOS(const QString& currentVersion,
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
    }
}

void UpdateNotifier::notifyUpdateAvailableManualMacOS(const QString& currentVersion,
                                                       const QString& latestVersion,
                                                       const QString& releaseUrl) {
    notifyUpdateAvailableAutoMacOS(currentVersion, latestVersion, releaseUrl);
}

void UpdateNotifier::notifyNoUpdateManualMacOS() {
    QMessageBox::information(nullptr,
        tr("Обновления"),
        tr("Вы используете последнюю версию."));
}

void UpdateNotifier::notifyCheckFailedManualMacOS(const QString& error) {
    QMessageBox::warning(nullptr,
        tr("Проверка обновлений"),
        tr("Не удалось проверить обновления: %1").arg(error));
}

void UpdateNotifier::notifyStartupResourcesUnavailableMacOS() {
    QMessageBox::warning(nullptr,
        tr("Невозможно выполнить команду"),
        tr("Ресурсы для запуска отсутствуют"));
}

#endif

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)

void UpdateNotifier::notifyUpdateAvailableAutoLinux(const QString& currentVersion,
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
    }
}

void UpdateNotifier::notifyUpdateAvailableManualLinux(const QString& currentVersion,
                                                       const QString& latestVersion,
                                                       const QString& releaseUrl) {
    notifyUpdateAvailableAutoLinux(currentVersion, latestVersion, releaseUrl);
}

void UpdateNotifier::notifyNoUpdateManualLinux() {
    QMessageBox::information(nullptr,
        tr("Обновления"),
        tr("Вы используете последнюю версию."));
}

void UpdateNotifier::notifyCheckFailedManualLinux(const QString& error) {
    QMessageBox::warning(nullptr,
        tr("Проверка обновлений"),
        tr("Не удалось проверить обновления: %1").arg(error));
}

void UpdateNotifier::notifyStartupResourcesUnavailableLinux() {
    QMessageBox::warning(nullptr,
        tr("Невозможно выполнить команду"),
        tr("Ресурсы для запуска отсутствуют"));
}

#endif
