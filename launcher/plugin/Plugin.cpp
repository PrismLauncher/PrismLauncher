// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "MTPixmapCache.h"
#include "Plugin.h"
#include "PluginContribution.h"
#include "api/PluginInterface.h"
#include "FileSystem.h"

Q_LOGGING_CATEGORY(pluginLogC, "launcher.plugins")

Plugin::Plugin(const QFileInfo& file)
{
    m_file_info = file;
    m_enabled = !QFileInfo(relativePath(".disabled")).exists();
    m_id = file.fileName();
}

Plugin::~Plugin()
{
    if (m_interface) {
        delete m_interface;
    }
    if (m_loader) {
        m_loader->unload();
        delete m_loader;
    }
}

bool Plugin::loadInfo()
{
    auto pluginInfoFile = FS::PathCombine(m_file_info.absoluteFilePath(), "plugin.json");

    QFile file(pluginInfoFile);

    // TODO: We should probably report this error to the user.
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << QString("Failed to read the plugin info file (%1).").arg(pluginInfoFile).toUtf8();
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    // Fail if the JSON is invalid.
    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << QString("Failed to parse plugin info file: %1 at offset %2")
                           .arg(parseError.errorString(), QString::number(parseError.offset))
                           .toUtf8();
        return false;
    }

    // Make sure the root is an object.
    if (!jsonDoc.isObject()) {
        qCritical() << "Invalid plugin info JSON: Root should be an array.";
        return false;
    }

    QJsonObject root = jsonDoc.object();

    auto pluginFormatVersion = root.value("formatVersion").toVariant().toInt();
    switch (pluginFormatVersion) {
        case 1: {
            loadV1(root);
            return true;
        }
        default: {
            QString newName = "plugin-old.json";
            qWarning() << "Unknown format version when loading plugin info. Existing one will be renamed to" << newName;
            // Attempt to rename the old version.
            file.rename(newName);
            return false;
        }
    }
}

static bool loadIconFile(const Plugin& plugin)
{
    QFileInfo icon_info(FS::PathCombine(plugin.fileinfo().filePath(), plugin.iconPath()));
    if (icon_info.exists() && icon_info.isFile()) {
        QFile icon(icon_info.filePath());
        if (!icon.open(QIODevice::ReadOnly))
            return false;
        auto data = icon.readAll();
        auto img = QImage::fromData(data);
        icon.close();
        if (!img.isNull()) {
            plugin.setIcon(img);
        }
        return false;
    }
    return false;
}

QPixmap Plugin::icon(QSize size, Qt::AspectRatioMode mode) const
{
    QPixmap cached_image;
    if (PixmapCache::find(m_image_cache_key.key, &cached_image)) {
        if (size.isNull())
            return cached_image;
        return cached_image.scaled(size, mode, Qt::SmoothTransformation);
    }

    // No valid image we can get
    if ((!m_image_cache_key.was_ever_used && m_image_cache_key.was_read_attempt) || iconPath().isEmpty())
        return {};

    if (m_image_cache_key.was_ever_used) {
        qDebug() << "Plugin" << name() << "Had it's icon evicted form the cache. reloading...";
        PixmapCache::markCacheMissByEviciton();
    }
    // Image got evicted from the cache or an attempt to load it has not been made. load it and retry.
    m_image_cache_key.was_read_attempt = true;
    loadIconFile(*this);
    return icon(size);
}

void Plugin::setIcon(QImage new_image) const
{
    QMutexLocker locker(&m_data_lock);

    Q_ASSERT(!new_image.isNull());

    if (m_image_cache_key.key.isValid())
        PixmapCache::remove(m_image_cache_key.key);

    // scale the image to avoid flooding the pixmapcache
    auto pixmap =
        QPixmap::fromImage(new_image.scaled({ 64, 64 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    m_image_cache_key.key = PixmapCache::insert(pixmap);
    m_image_cache_key.was_ever_used = true;
    m_image_cache_key.was_read_attempt = true;
}

QString Plugin::relativePath(QString path) const
{
    return FS::PathCombine(m_file_info.absoluteFilePath(), path);
}

void Plugin::enable(EnableAction action)
{
    bool enable = true;
    switch (action) {
        case EnableAction::ENABLE:
            enable = true;
            break;
        case EnableAction::DISABLE:
            enable = false;
            break;
        case EnableAction::TOGGLE:
        default:
            enable = !enabled();
            break;
    }

    if (m_enabled == enable)
        return;

    QString path = relativePath(".disabled");
    auto file = QFile(path);
    if (enable) {
        if (!file.exists())
            return;

        file.remove();

        onEnable();

    } else {
        file.open(QIODevice::NewOnly | QIODevice::WriteOnly);
        file.close();

        onDisable();

        // TODO: m_needsRestart should be dynamically decided by the plugin itself
        m_needsRestart = true;
    }

    m_enabled = enable;
}

void Plugin::onEnable()
{
    // TODO: make enable better; i.e. settings window needs to be reloaded etc.

    qInfo(pluginLogC) << "Enable plugin" << m_id;
    for (auto& contribution : m_contributions) {
        contribution->onPluginEnable();
    }

    QString nativePluginPath = getNativePluginPath();
    if (nativePluginPath.isEmpty())
        return;

    nativePluginPath = relativePath(nativePluginPath);
    if (!QFileInfo(nativePluginPath).exists()) {
        qWarning(pluginLogC) << "Failed loading native plugin" << nativePluginPath << ": file does not exists";
        return;
    }

    qInfo(pluginLogC) << "Try loading native plugin" << nativePluginPath;

    m_loader = new QPluginLoader(nativePluginPath);

    QObject* object = m_loader->instance();
    if (object) {
        m_interface = qobject_cast<PluginInterface*>(object);
        if (!m_interface) {
            qCritical(pluginLogC) << "Failed loading native plugin; reason:" << m_loader->errorString();
            m_loader->unload();
            delete m_loader;
            m_loader = nullptr;
            return;
        }

        m_interface->onEnable(*this);
    } else {
        qCritical(pluginLogC) << "Failed loading native plugin; reason:" << m_loader->errorString();
        m_loader->unload();
        delete m_loader;
        m_loader = nullptr;
    }
}

void Plugin::onDisable()
{
    // TODO: make plugins disable- / unload-able

    for (auto& contribution : m_contributions) {
        contribution->onPluginDisable();
    }

    if (m_interface) {
        m_interface->onDisable(*this);
        delete m_interface;
        m_loader->unload();
        delete m_loader;
        m_interface = nullptr;
        m_loader = nullptr;
    }
}

void Plugin::loadV1(const QJsonObject& root)
{
    m_name = root.value("displayName").toString();
    m_desc = root.value("description").toString();
    m_metaurl = root.value("homepage").toString();
    m_icon_file = root.value("icon").toString();
    m_issueTracker = root.value("issues").toString();
    m_license = root.value("license").toString();

    if (root.value("natives").isObject()) {
        auto nativesJson = root.value("natives").toObject();
        m_native_plugin_paths.osx = nativesJson.value("osx").toString();
        m_native_plugin_paths.win32 = nativesJson.value("win32").toString();
        m_native_plugin_paths.win64 = nativesJson.value("win64").toString();
        m_native_plugin_paths.lin32 = nativesJson.value("linux32").toString();
        m_native_plugin_paths.lin64 = nativesJson.value("linux64").toString();
    }

    if (root.value("authors").isArray()) {
        auto authorsJson = root.value("authors").toArray();
        for (auto authorName : authorsJson) {
            if (!authorName.isString())
                continue;

            m_authors.append(authorName.toString());
        }
    }

    ExtentionPointRegistry& registry = ExtentionPointRegistry::instance();

    int skipped = 0, failed = 0;
    QJsonObject contributions = root.value("contributions").toObject();
    for (auto key : contributions.keys()) {
        if (!registry.isKnown(key)) {
            qWarning(pluginLogC) << "Got unknown contribution kind" << key;
            continue;
        }

        QJsonArray entries = contributions[key].toArray();
        registry.withFactory(key, [this, &entries, &skipped, &failed] (ExtentionPointRegistry::Factory& factory) {
            for (auto entry : entries) {
                PluginContributionPtr contribution;
                contribution.reset(factory());
                if (!contribution) { skipped++; continue; }
                if (!contribution->loadConfig(*this, entry)) { failed++; continue; }
                m_contributions.append(contribution);
            }
        });
    }

    qInfo(pluginLogC) << "Loaded" << m_contributions.size() << "contributions from plugin" << id();
    if (skipped > 0)
        qWarning(pluginLogC) << "Skipped" << skipped << "contributions for plugin" << id();
    if (failed > 0)
        qCritical(pluginLogC) << "Failed" << failed << "contributions for plugin" << id();
}

QString Plugin::getNativePluginPath() {
    #if defined(Q_OS_WIN)
        #if defined(Q_PROCESSOR_X86_64)
            if (!m_native_plugin_paths.win64.isEmpty()) {
                return m_native_plugin_paths.win64;
            }
        #endif
        return m_native_plugin_paths.win32;
    #elif defined(Q_OS_LINUX)
        #if defined(Q_PROCESSOR_X86_64)
            if (!m_native_plugin_paths.lin64.isEmpty()) {
                return m_native_plugin_paths.lin64;
            }
        #endif
        return m_native_plugin_paths.lin32;
    #elif defined(Q_OS_MACOS)
        return m_native_plugin_paths.osx;
    #else
        qWarning() << "Native plugins not supported! unknown OS";
        return "";
    #endif
}
