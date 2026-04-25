#pragma once

#include <QObject>
#include <QString>

class QSystemTrayIcon;

class UpdateNotifier : public QObject {
    Q_OBJECT
public:
    explicit UpdateNotifier(QSystemTrayIcon* trayIcon, QObject* parent = nullptr);
    ~UpdateNotifier() = default;

    bool notifyUpdateAvailableAuto(const QString& currentVersion,
                                    const QString& latestVersion,
                                    const QString& releaseUrl);

    bool notifyUpdateAvailableManual(const QString& currentVersion,
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
    bool notifyUpdateAvailableAutoWindows(const QString& currentVersion,
                                           const QString& latestVersion,
                                           const QString& releaseUrl);
    bool notifyUpdateAvailableManualWindows(const QString& currentVersion,
                                             const QString& latestVersion,
                                             const QString& releaseUrl);
    void notifyNoUpdateManualWindows();
    void notifyCheckFailedManualWindows(const QString& error);
    void notifyStartupResourcesUnavailableWindows();
#elif defined(Q_OS_MACOS)
    bool notifyUpdateAvailableAutoMacOS(const QString& currentVersion,
                                         const QString& latestVersion,
                                         const QString& releaseUrl);
    bool notifyUpdateAvailableManualMacOS(const QString& currentVersion,
                                           const QString& latestVersion,
                                           const QString& releaseUrl);
    void notifyNoUpdateManualMacOS();
    void notifyCheckFailedManualMacOS(const QString& error);
    void notifyStartupResourcesUnavailableMacOS();
#else
    bool notifyUpdateAvailableAutoLinux(const QString& currentVersion,
                                         const QString& latestVersion,
                                         const QString& releaseUrl);
    bool notifyUpdateAvailableManualLinux(const QString& currentVersion,
                                           const QString& latestVersion,
                                           const QString& releaseUrl);
    void notifyNoUpdateManualLinux();
    void notifyCheckFailedManualLinux(const QString& error);
    void notifyStartupResourcesUnavailableLinux();
#endif
};
