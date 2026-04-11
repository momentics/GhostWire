#include "UpdateChecker.h"
#include "Config.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QDateTime>
#include <QRegularExpression>

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
    if (!m_ignoreCache && !shouldCheckNow())
        return;
    // https://api.github.com/repos/momentics/GhostWire/releases/latest
    QUrl url(QString("https://api.github.com/repos/%1/%2/releases/latest")
        .arg(m_repoOwner, m_repoName));

    qDebug() << "UpdateChecker: запрос" << url.toString();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
        QString("GhostWireDesktop/%1").arg(m_currentVersion));
    request.setRawHeader("Accept", "application/vnd.github+json");

    QString etag = loadEtag();
    if (!etag.isEmpty())
        request.setRawHeader("If-None-Match", etag.toUtf8());

    QNetworkReply* reply = m_net.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
        reply->deleteLater();
    });
}

void UpdateChecker::checkForUpdatesNow() {
    m_ignoreCache = true;
    checkForUpdates();
}

bool UpdateChecker::shouldCheckNow() {
    qint64 last = lastCheckTime();
    if (last == 0) return true;
    qint64 hoursSince = QDateTime::currentSecsSinceEpoch() - last;
    return hoursSince >= Config::UPDATE_CHECK_INTERVAL_HOURS * 3600;
}

qint64 UpdateChecker::lastCheckTime() const {
    QSettings settings("GhostWire", "GhostWireDesktop");
    return settings.value("updateCheckLastTime", 0).toLongLong();
}

void UpdateChecker::saveCheckTime(const QString& etag) {
    QSettings settings("GhostWire", "GhostWireDesktop");
    settings.setValue("updateCheckLastTime", QDateTime::currentSecsSinceEpoch());
    if (!etag.isEmpty())
        settings.setValue("updateCheckEtag", etag);
}

QString UpdateChecker::loadEtag() const {
    QSettings settings("GhostWire", "GhostWireDesktop");
    return settings.value("updateCheckEtag").toString();
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

void UpdateChecker::onReplyFinished(QNetworkReply* reply) {
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "UpdateChecker: ответ, статус=" << status;

    if (status == 304) {
        saveCheckTime(QString());
        if (m_ignoreCache)
            emit noUpdate();
        m_ignoreCache = false;
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
        if (m_ignoreCache)
            emit checkFailed(err);
        m_ignoreCache = false;
        return;
    }

    QString etag = reply->rawHeader("ETag");
    QByteArray body = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (m_ignoreCache)
            emit checkFailed("JSON parse error");
        m_ignoreCache = false;
        return;
    }

    QJsonObject obj = doc.object();
    QString tag = obj.value("tag_name").toString();
    QString htmlUrl = obj.value("html_url").toString();

    if (tag.isEmpty()) {
        if (m_ignoreCache)
            emit checkFailed("Empty tag_name");
        m_ignoreCache = false;
        return;
    }

    QString remoteVersion = normalizeTag(tag);
    SemVer remote = parseSemVer(remoteVersion);
    SemVer current = parseSemVer(m_currentVersion);

    saveCheckTime(etag);

    if (compareSemVer(remote, current) > 0) {
        emit updateAvailable(remoteVersion, htmlUrl);
    } else {
        if (m_ignoreCache)
            emit noUpdate();
    }
    m_ignoreCache = false;
}
