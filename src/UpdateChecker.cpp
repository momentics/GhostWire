#include "UpdateChecker.h"
#include "Config.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>

UpdateChecker::UpdateChecker(const QString& currentVersion,
                             const QString& repoOwner,
                             const QString& repoName,
                             QObject* parent)
    : QObject(parent)
    , m_currentVersion(currentVersion)
    , m_repoOwner(repoOwner)
    , m_repoName(repoName)
{
}

void UpdateChecker::checkForUpdates() {
    startCheck(false, false);
}

void UpdateChecker::checkForUpdatesOnStartup() {
    startCheck(false, true);
}

void UpdateChecker::checkForUpdatesNow() {
    startCheck(true, true);
}

void UpdateChecker::startCheck(bool manual, bool ignoreSchedule) {
    if (m_checkInProgress) {
        if (manual)
            m_manualCheckQueued = true;
        return;
    }

    if (!manual && isPostponed())
        return;

    if (!ignoreSchedule && !shouldCheckNow())
        return;

    // https://api.github.com/repos/momentics/GhostWire/releases/latest
    QUrl url(QString("https://api.github.com/repos/%1/%2/releases/latest")
        .arg(m_repoOwner, m_repoName));

    qDebug() << "UpdateChecker: запрос" << url.toString();
    m_checkInProgress = true;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
        QString("GhostWireDesktop/%1").arg(m_currentVersion));
    request.setRawHeader("Accept", "application/vnd.github+json");

    QString etag = loadEtag();
    if (!etag.isEmpty())
        request.setRawHeader("If-None-Match", etag.toUtf8());

    QString lastModified = loadLastModified();
    if (!lastModified.isEmpty())
        request.setRawHeader("If-Modified-Since", lastModified.toUtf8());

    QNetworkReply* reply = m_net.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manual]() {
        onReplyFinished(reply, manual);
        reply->deleteLater();
        m_checkInProgress = false;

        if (m_manualCheckQueued) {
            m_manualCheckQueued = false;
            startCheck(true, true);
        }
    });
}

bool UpdateChecker::shouldCheckNow() {
    qint64 last = lastCheckTime();
    if (last == 0) return true;
    qint64 hoursSince = QDateTime::currentSecsSinceEpoch() - last;
    return hoursSince >= Config::UPDATE_CHECK_INTERVAL_HOURS * 3600;
}

int UpdateChecker::millisecondsUntilNextCheck() const {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 postponed = postponedUntil();
    if (postponed > now)
        return static_cast<int>((postponed - now) * 1000);

    const qint64 last = lastCheckTime();
    if (last == 0)
        return 0;

    const qint64 intervalSecs = Config::UPDATE_CHECK_INTERVAL_HOURS * 3600;
    const qint64 elapsedSecs = QDateTime::currentSecsSinceEpoch() - last;
    if (elapsedSecs <= 0)
        return static_cast<int>(intervalSecs * 1000);
    if (elapsedSecs >= intervalSecs)
        return 0;

    return static_cast<int>((intervalSecs - elapsedSecs) * 1000);
}

void UpdateChecker::postponeUpdateNotification() {
    QSettings settings("GhostWire", "GhostWireDesktop");
    const qint64 postponedUntilSecs = QDateTime::currentSecsSinceEpoch()
        + Config::UPDATE_CHECK_DECLINED_COOLDOWN_HOURS * 3600;
    settings.setValue("updateCheckPostponedUntil", postponedUntilSecs);
}

void UpdateChecker::clearUpdateNotificationPostponement() {
    QSettings settings("GhostWire", "GhostWireDesktop");
    settings.remove("updateCheckPostponedUntil");
}

qint64 UpdateChecker::lastCheckTime() const {
    QSettings settings("GhostWire", "GhostWireDesktop");
    return settings.value("updateCheckLastTime", 0).toLongLong();
}

qint64 UpdateChecker::postponedUntil() const {
    QSettings settings("GhostWire", "GhostWireDesktop");
    return settings.value("updateCheckPostponedUntil", 0).toLongLong();
}

bool UpdateChecker::isPostponed() const {
    return postponedUntil() > QDateTime::currentSecsSinceEpoch();
}

void UpdateChecker::saveCheckTime(const QString& etag, const QString& lastModified) {
    QSettings settings("GhostWire", "GhostWireDesktop");
    settings.setValue("updateCheckLastTime", QDateTime::currentSecsSinceEpoch());
    if (!etag.isEmpty())
        settings.setValue("updateCheckEtag", etag);
    if (!lastModified.isEmpty())
        settings.setValue("updateCheckLastModified", lastModified);
}

void UpdateChecker::saveLatestRelease(const QString& latestVersion,
                                      const QString& releaseUrl,
                                      const QString& etag,
                                      const QString& lastModified) {
    QSettings settings("GhostWire", "GhostWireDesktop");
    settings.setValue("updateCheckLastTime", QDateTime::currentSecsSinceEpoch());
    settings.setValue("updateCheckLatestVersion", latestVersion);
    settings.setValue("updateCheckReleaseUrl", releaseUrl);
    if (!etag.isEmpty())
        settings.setValue("updateCheckEtag", etag);
    if (!lastModified.isEmpty())
        settings.setValue("updateCheckLastModified", lastModified);
}

UpdateInfo UpdateChecker::loadLatestRelease() const {
    QSettings settings("GhostWire", "GhostWireDesktop");
    const QString latestVersion = settings.value("updateCheckLatestVersion").toString();
    const QString releaseUrl = settings.value("updateCheckReleaseUrl").toString();

    if (latestVersion.isEmpty())
        return UpdateInfo();

    return evaluateRelease(latestVersion, releaseUrl);
}

QString UpdateChecker::loadEtag() const {
    QSettings settings("GhostWire", "GhostWireDesktop");
    return settings.value("updateCheckEtag").toString();
}

QString UpdateChecker::loadLastModified() const {
    QSettings settings("GhostWire", "GhostWireDesktop");
    return settings.value("updateCheckLastModified").toString();
}

SemVer UpdateChecker::parseSemVer(const QString& version) {
    SemVer result;
    QRegularExpression re(R"((\d+)\.(\d+)\.(\d+))");
    QRegularExpressionMatch m = re.match(version);
    if (m.hasMatch()) {
        result.major = m.captured(1).toInt();
        result.minor = m.captured(2).toInt();
        result.patch = m.captured(3).toInt();
    }
    return result;
}

int UpdateChecker::compareSemVer(const SemVer& a, const SemVer& b) {
    if (a.major != b.major) return a.major - b.major;
    if (a.minor != b.minor) return a.minor - b.minor;
    return a.patch - b.patch;
}

QString UpdateChecker::normalizeTag(const QString& tag) {
    QString t = tag.trimmed();
    if (t.startsWith('v') || t.startsWith('V'))
        t = t.mid(1);
    return t;
}

UpdateInfo UpdateChecker::evaluateRelease(const QString& remoteVersion,
                                          const QString& releaseUrl) const {
    UpdateInfo info;
    info.latestVersion = remoteVersion;
    info.releaseUrl = releaseUrl;

    SemVer remote = parseSemVer(remoteVersion);
    SemVer current = parseSemVer(m_currentVersion);
    info.updateAvailable = compareSemVer(remote, current) > 0;
    return info;
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply, bool manual) {
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "UpdateChecker: ответ, статус=" << status;
    const bool suppressAutomaticResult = !manual && m_manualCheckQueued;

    if (status == 304) {
        UpdateInfo cached = loadLatestRelease();
        if (cached.latestVersion.isEmpty()) {
            QSettings settings("GhostWire", "GhostWireDesktop");
            settings.remove("updateCheckEtag");
            settings.remove("updateCheckLastModified");
            if (manual)
                emit checkFailed("Cached release metadata unavailable", manual);
            return;
        }

        saveCheckTime(QString(), QString());
        if (cached.updateAvailable && !suppressAutomaticResult) {
            emit updateAvailable(cached.latestVersion, cached.releaseUrl, manual);
        } else if (manual) {
            emit noUpdate(manual);
        }
        return;
    }

    if (status != 200) {
        QString err;
        if (status == 0) {
            err = reply->errorString();
        } else {
            err = QString("HTTP %1").arg(status);
        }
        qDebug() << "UpdateChecker: ошибка —" << err;
        if (manual)
            emit checkFailed(err, manual);
        return;
    }

    QString etag = reply->rawHeader("ETag");
    QString lastModified = reply->rawHeader("Last-Modified");
    QByteArray body = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (manual)
            emit checkFailed("JSON parse error", manual);
        return;
    }

    QJsonObject obj = doc.object();
    QString tag = obj.value("tag_name").toString();
    QString htmlUrl = obj.value("html_url").toString();

    if (tag.isEmpty()) {
        if (manual)
            emit checkFailed("Empty tag_name", manual);
        return;
    }

    QString remoteVersion = normalizeTag(tag);
    UpdateInfo info = evaluateRelease(remoteVersion, htmlUrl);
    saveLatestRelease(remoteVersion, htmlUrl, etag, lastModified);

    if (info.updateAvailable && !suppressAutomaticResult) {
        emit updateAvailable(info.latestVersion, info.releaseUrl, manual);
    } else {
        if (manual)
            emit noUpdate(manual);
    }
}
