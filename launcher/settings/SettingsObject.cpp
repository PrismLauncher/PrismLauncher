/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "settings/SettingsObject.h"
#include <QDebug>
#include "PassthroughSetting.h"
#include "settings/OverrideSetting.h"
#include "settings/Setting.h"

#include <QDir>
#include <QVariant>
#include <utility>

#ifdef Q_OS_MACOS
#include "macsandbox/SecurityBookmarkFileAccess.h"
#endif

SettingsObject::SettingsObject(QObject* parent) : QObject(parent) {}

SettingsObject::~SettingsObject()
{
    m_settings.clear();
}

std::shared_ptr<Setting> SettingsObject::registerOverride(std::shared_ptr<Setting> original, std::shared_ptr<Setting> gate)
{
    if (contains(original->id())) {
        qCritical() << QString("Failed to register setting %1. ID already exists.").arg(original->id());
        return nullptr;  // Fail
    }
    auto override = std::make_shared<OverrideSetting>(original, gate);
    override->m_storage = this;
    connectSignals(*override);
    m_settings.insert(override->id(), override);
    return override;
}

std::shared_ptr<Setting> SettingsObject::registerPassthrough(std::shared_ptr<Setting> original, std::shared_ptr<Setting> gate)
{
    if (contains(original->id())) {
        qCritical() << QString("Failed to register setting %1. ID already exists.").arg(original->id());
        return nullptr;  // Fail
    }
    auto passthrough = std::make_shared<PassthroughSetting>(original, gate);
    passthrough->m_storage = this;
    connectSignals(*passthrough);
    m_settings.insert(passthrough->id(), passthrough);
    return passthrough;
}

std::shared_ptr<Setting> SettingsObject::registerSetting(QStringList synonyms, QVariant defVal)
{
    if (synonyms.empty())
        return nullptr;
    if (contains(synonyms.first())) {
        qCritical() << QString("Failed to register setting %1. ID already exists.").arg(synonyms.first());
        return nullptr;  // Fail
    }
    auto setting = std::make_shared<Setting>(synonyms, defVal);
    setting->m_storage = this;
    connectSignals(*setting);
    m_settings.insert(setting->id(), setting);
    return setting;
}

std::shared_ptr<Setting> SettingsObject::getSetting(const QString& id) const
{
    // Make sure there is a setting with the given ID.
    if (!m_settings.contains(id))
        return NULL;

    return m_settings[id];
}

QVariant SettingsObject::get(const QString& id)
{
    auto setting = getSetting(id);

#ifdef Q_OS_MACOS
    // for macOS, use a security scoped bookmark for the paths
    if (id.endsWith("Dir")) {
        return { getPathFromBookmark(id) };
    }
#endif

    return (setting ? setting->get() : QVariant());
}

bool SettingsObject::set(const QString& id, QVariant value)
{
    auto setting = getSetting(id);
    if (!setting) {
        qCritical() << QString("Error changing setting %1. Setting doesn't exist.").arg(id);
        return false;
    }

#ifdef Q_OS_MACOS
    // for macOS, keep a security scoped bookmark for the paths
    if (value.userType() == QMetaType::QString && id.endsWith("Dir")) {
        setPathWithBookmark(id, value.toString());
    }
#endif

    setting->set(std::move(value));
    return true;
}

#ifdef Q_OS_MACOS
QString SettingsObject::getPathFromBookmark(const QString& id)
{
    auto setting = getSetting(id);
    if (!setting) {
        qCritical() << QString("Error changing setting %1. Setting doesn't exist.").arg(id);
        return "";
    }

    // there is no need to use bookmarks if the default value is used or the directory is within the data directory (already can access)
    if (setting->get() == setting->defValue() ||
        QDir(setting->get().toString()).absolutePath().startsWith(QDir::current().absolutePath())) {
        return setting->get().toString();
    }

    auto bookmarkId = id + "Bookmark";
    auto bookmarkSetting = getSetting(bookmarkId);
    if (!bookmarkSetting) {
        qCritical() << QString("Error changing setting %1. Bookmark setting doesn't exist.").arg(id);
        return "";
    }

    QByteArray bookmark = bookmarkSetting->get().toByteArray();
    if (bookmark.isEmpty()) {
        qDebug() << "Creating bookmark for" << id << "at" << setting->get().toString();
        setPathWithBookmark(id, setting->get().toString());
        return setting->get().toString();
    }
    bool stale;
    QUrl url = m_sandboxedFileAccess.securityScopedBookmarkToURL(bookmark, stale);
    if (url.isValid()) {
        if (stale) {
            setting->set(url.path());
            bookmarkSetting->set(bookmark);
        }

        m_sandboxedFileAccess.startUsingSecurityScopedBookmark(bookmark, stale);
        // already did a stale check, no need to do it again

        // convert to relative path to current directory if `url` is a descendant of the current directory
        QDir currentDir = QDir::current().absolutePath();
        return url.path().startsWith(currentDir.absolutePath()) ? currentDir.relativeFilePath(url.path()) : url.path();
    }

    return setting->get().toString();
}

bool SettingsObject::setPathWithBookmark(const QString& id, const QString& path)
{
    auto setting = getSetting(id);
    if (!setting) {
        qCritical() << QString("Error changing setting %1. Setting doesn't exist.").arg(id);
        return false;
    }

    QDir dir(path);
    if (!dir.exists()) {
        qCritical() << QString("Error changing setting %1. Path doesn't exist.").arg(id);
        return false;
    }
    QString absolutePath = dir.absolutePath();
    QString bookmarkId = id + "Bookmark";
    std::shared_ptr<Setting> bookmarkSetting = getSetting(bookmarkId);
    // there is no need to use bookmarks if the default value is used or the directory is within the data directory (already can access)
    if (path == setting->defValue().toString() || absolutePath.startsWith(QDir::current().absolutePath())) {
        bookmarkSetting->reset();
        return true;
    }
    QByteArray bytes = m_sandboxedFileAccess.pathToSecurityScopedBookmark(absolutePath);
    if (bytes.isEmpty()) {
        qCritical() << QString("Failed to create bookmark for %1 - no access?").arg(id);
        // TODO: show an alert to the user asking them to reselect the directory
        return false;
    }
    auto oldBookmark = bookmarkSetting->get().toByteArray();
    m_sandboxedFileAccess.stopUsingSecurityScopedBookmark(oldBookmark);
    if (!bytes.isEmpty() && bookmarkSetting) {
        bookmarkSetting->set(bytes);
        bool stale;
        m_sandboxedFileAccess.startUsingSecurityScopedBookmark(bytes, stale);
        // just created the bookmark, it shouldn't be stale
    }

    setting->set(path);
    return true;
}
#endif

void SettingsObject::reset(const QString& id) const
{
    auto setting = getSetting(id);
    if (setting)
        setting->reset();
}

bool SettingsObject::contains(const QString& id)
{
    return m_settings.contains(id);
}

bool SettingsObject::reload()
{
    for (auto setting : m_settings.values()) {
        setting->set(setting->get());
    }
    return true;
}

void SettingsObject::connectSignals(const Setting& setting)
{
    connect(&setting, &Setting::SettingChanged, this, &SettingsObject::changeSetting);
    connect(&setting, &Setting::SettingChanged, this, &SettingsObject::SettingChanged);

    connect(&setting, &Setting::settingReset, this, &SettingsObject::resetSetting);
    connect(&setting, &Setting::settingReset, this, &SettingsObject::settingReset);
}

std::shared_ptr<Setting> SettingsObject::getOrRegisterSetting(const QString& id, QVariant defVal)
{
    return contains(id) ? getSetting(id) : registerSetting(id, defVal);
}
