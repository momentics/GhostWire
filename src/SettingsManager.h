#pragma once

#include <QString>

class QSettings;

class SettingsManager {
public:
    SettingsManager();

    bool getProxyRunning() const;
    void setProxyRunning(bool value);

    qint64 getUpdateCheckLastTime() const;
    void setUpdateCheckLastTime(qint64 value);
    qint64 getUpdateCheckPostponedUntil() const;
    void setUpdateCheckPostponedUntil(qint64 value);
    void removeUpdateCheckPostponedUntil();
    QString getUpdateCheckEtag() const;
    void setUpdateCheckEtag(const QString& value);
    QString getUpdateCheckLastModified() const;
    void setUpdateCheckLastModified(const QString& value);
    QString getUpdateCheckLatestVersion() const;
    void setUpdateCheckLatestVersion(const QString& value);
    QString getUpdateCheckReleaseUrl() const;
    void setUpdateCheckReleaseUrl(const QString& value);
    void removeUpdateCheckEtag();
    void removeUpdateCheckLastModified();

private:
    QSettings* m_settings;
};
