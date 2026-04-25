#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct SemVer {
    int major = 0;
    int minor = 0;
    int patch = 0;
};

struct UpdateInfo {
    bool updateAvailable = false;
    QString latestVersion;
    QString releaseUrl;
};

class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(const QString& currentVersion,
                           const QString& repoOwner,
                           const QString& repoName,
                           QObject* parent = nullptr);
    ~UpdateChecker() = default;

    void checkForUpdates();
    void checkForUpdatesOnStartup();
    void checkForUpdatesNow();
    int millisecondsUntilNextCheck() const;
    void postponeUpdateNotification();
    void clearUpdateNotificationPostponement();

signals:
    void updateAvailable(const QString& version, const QString& releaseUrl, bool manual);
    void noUpdate(bool manual);
    void checkFailed(const QString& error, bool manual);

private slots:
    void onReplyFinished(QNetworkReply* reply, bool manual);

private:
    QNetworkAccessManager m_net;
    QString m_currentVersion;
    QString m_repoOwner;
    QString m_repoName;
    bool m_checkInProgress = false;
    bool m_manualCheckQueued = false;

    static SemVer parseSemVer(const QString& version);
    static int compareSemVer(const SemVer& a, const SemVer& b);
    static QString normalizeTag(const QString& tag);

    void startCheck(bool manual, bool ignoreSchedule);
    UpdateInfo evaluateRelease(const QString& remoteVersion, const QString& releaseUrl) const;
    bool isPostponed() const;
    bool shouldCheckNow();
    void saveCheckTime(const QString& etag, const QString& lastModified);
    void saveLatestRelease(const QString& latestVersion,
                           const QString& releaseUrl,
                           const QString& etag,
                           const QString& lastModified);
    UpdateInfo loadLatestRelease() const;
    qint64 postponedUntil() const;
    QString loadEtag() const;
    QString loadLastModified() const;
    qint64 lastCheckTime() const;
};
