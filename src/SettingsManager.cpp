#include "SettingsManager.h"
#include <QSettings>

SettingsManager::SettingsManager()
    : m_settings(std::make_unique<QSettings>("GhostWire", "GhostWireDesktop"))
{
}

bool SettingsManager::getProxyRunning() const {
    return m_settings->value("proxyRunning", false).toBool();
}

void SettingsManager::setProxyRunning(bool value) {
    m_settings->setValue("proxyRunning", value);
}

qint64 SettingsManager::getUpdateCheckLastTime() const {
    return m_settings->value("updateCheckLastTime", 0).toLongLong();
}

void SettingsManager::setUpdateCheckLastTime(qint64 value) {
    m_settings->setValue("updateCheckLastTime", value);
}

qint64 SettingsManager::getUpdateCheckPostponedUntil() const {
    return m_settings->value("updateCheckPostponedUntil", 0).toLongLong();
}

void SettingsManager::setUpdateCheckPostponedUntil(qint64 value) {
    m_settings->setValue("updateCheckPostponedUntil", value);
}

void SettingsManager::removeUpdateCheckPostponedUntil() {
    m_settings->remove("updateCheckPostponedUntil");
}

QString SettingsManager::getUpdateCheckEtag() const {
    return m_settings->value("updateCheckEtag").toString();
}

void SettingsManager::setUpdateCheckEtag(const QString& value) {
    m_settings->setValue("updateCheckEtag", value);
}

QString SettingsManager::getUpdateCheckLastModified() const {
    return m_settings->value("updateCheckLastModified").toString();
}

void SettingsManager::setUpdateCheckLastModified(const QString& value) {
    m_settings->setValue("updateCheckLastModified", value);
}

QString SettingsManager::getUpdateCheckLatestVersion() const {
    return m_settings->value("updateCheckLatestVersion").toString();
}

void SettingsManager::setUpdateCheckLatestVersion(const QString& value) {
    m_settings->setValue("updateCheckLatestVersion", value);
}

QString SettingsManager::getUpdateCheckReleaseUrl() const {
    return m_settings->value("updateCheckReleaseUrl").toString();
}

void SettingsManager::setUpdateCheckReleaseUrl(const QString& value) {
    m_settings->setValue("updateCheckReleaseUrl", value);
}

void SettingsManager::removeUpdateCheckEtag() {
    m_settings->remove("updateCheckEtag");
}

void SettingsManager::removeUpdateCheckLastModified() {
    m_settings->remove("updateCheckLastModified");
}
