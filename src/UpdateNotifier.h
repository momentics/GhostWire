#pragma once

#include <QObject>
#include <QString>

class QSystemTrayIcon;

class UpdateNotifier : public QObject {
    Q_OBJECT
public:
    explicit UpdateNotifier(QSystemTrayIcon* trayIcon, QObject* parent = nullptr);
    ~UpdateNotifier();

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
    QString m_pendingReleaseUrl;

    /// Показать диалог обновления для платформ с Qt MessageBox fallback.
    /// Возвращает true если пользователь открыл страницу.
    bool showUpdateAvailableDialog(const QString& currentVersion,
                                    const QString& latestVersion,
                                    const QString& releaseUrl);

    /// Показать информационное сообщение «Обновлений нет».
    void showNoUpdateInfo();

    /// Показать предупреждение об ошибке проверки.
    void showCheckFailedWarning(const QString& error);
};
