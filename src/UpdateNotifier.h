#pragma once

#include <QObject>
#include <QString>

class QSystemTrayIcon;

class UpdateNotifier : public QObject {
    Q_OBJECT
public:
    explicit UpdateNotifier(QSystemTrayIcon* trayIcon, QObject* parent = nullptr);
    ~UpdateNotifier() = default;

    void notifyUpdateAvailableAuto(const QString& currentVersion,
                                    const QString& latestVersion,
                                    const QString& releaseUrl);

    void notifyUpdateAvailableManual(const QString& currentVersion,
                                      const QString& latestVersion,
                                      const QString& releaseUrl);

    void notifyNoUpdateManual();

    void notifyCheckFailedManual(const QString& error);

    void notifyStartupResourcesUnavailable();

signals:
    void openReleaseUrl(const QString& url);

private:
    QSystemTrayIcon* m_trayIcon;

#ifdef Q_OS_WIN
    void notifyUpdateAvailableAutoWindows(const QString& currentVersion,
                                           const QString& latestVersion,
                                           const QString& releaseUrl);
    void notifyUpdateAvailableManualWindows(const QString& currentVersion,
                                             const QString& latestVersion,
                                             const QString& releaseUrl);
    void notifyNoUpdateManualWindows();
    void notifyCheckFailedManualWindows(const QString& error);
    void notifyStartupResourcesUnavailableWindows();
#elif defined(Q_OS_MACOS)
    void notifyUpdateAvailableAutoMacOS(const QString& currentVersion,
                                         const QString& latestVersion,
                                         const QString& releaseUrl);
    void notifyUpdateAvailableManualMacOS(const QString& currentVersion,
                                           const QString& latestVersion,
                                           const QString& releaseUrl);
    void notifyNoUpdateManualMacOS();
    void notifyCheckFailedManualMacOS(const QString& error);
    void notifyStartupResourcesUnavailableMacOS();
#else
    void notifyUpdateAvailableAutoLinux(const QString& currentVersion,
                                         const QString& latestVersion,
                                         const QString& releaseUrl);
    void notifyUpdateAvailableManualLinux(const QString& currentVersion,
                                           const QString& latestVersion,
                                           const QString& releaseUrl);
    void notifyNoUpdateManualLinux();
    void notifyCheckFailedManualLinux(const QString& error);
    void notifyStartupResourcesUnavailableLinux();
#endif
};
