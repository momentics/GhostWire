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
    void checkForUpdatesNow();

signals:
    void updateAvailable(const QString& version, const QString& releaseUrl);
    void noUpdate();
    void checkFailed(const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager m_net;
    QString m_currentVersion;
    QString m_repoOwner;
    QString m_repoName;
    bool m_ignoreCache = false;

    static SemVer parseSemVer(const QString& version);
    static int compareSemVer(const SemVer& a, const SemVer& b);
    static QString normalizeTag(const QString& tag);

    bool shouldCheckNow();
    void saveCheckTime(const QString& etag);
    QString loadEtag() const;
    qint64 lastCheckTime() const;
};
