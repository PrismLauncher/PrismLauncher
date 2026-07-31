// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
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
 */

#include "DevelopingModWatcher.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSet>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModFolderModel.h"
#include "settings/Setting.h"

Q_LOGGING_CATEGORY(developingModLog, "launcher.developingmod")

namespace {
constexpr int kDebounceMs = 900;
constexpr int kPollMs = 1000;
constexpr int kWriteSettleMs = 600;
}  // namespace

QStringList DevelopingModWatcher::defaultIgnorePatterns()
{
    return { QStringLiteral("*-sources.jar"), QStringLiteral("*-javadoc.jar"), QStringLiteral("*-dev.jar"),
             QStringLiteral("*-api.jar"),     QStringLiteral("*-thin.jar") };
}

DevelopingModWatcher::DevelopingModWatcher(MinecraftInstance* instance) : QObject(instance), m_instance(instance)
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(kDebounceMs);
    connect(&m_debounce, &QTimer::timeout, this, &DevelopingModWatcher::performSync);

    m_poll.setInterval(kPollMs);
    connect(&m_poll, &QTimer::timeout, this, &DevelopingModWatcher::pollSources);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &DevelopingModWatcher::directoryChanged);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &DevelopingModWatcher::fileChanged);

    auto* settings = m_instance->settings();
    auto reconnect = [this](std::shared_ptr<Setting> setting) {
        connect(setting.get(), &Setting::SettingChanged, this, [this] { reloadFromSettings(); });
    };
    reconnect(settings->getSetting("DevelopingModEnabled"));
    reconnect(settings->getSetting("DevelopingModFolders"));
    reconnect(settings->getSetting("DevelopingModIgnorePatterns"));

    reloadFromSettings();
}

void DevelopingModWatcher::setStatus(const QString& status)
{
    if (m_statusText == status)
        return;
    m_statusText = status;
    emit statusChanged(m_statusText);
}

QStringList DevelopingModWatcher::readStringListSetting(const QString& key) const
{
    const auto raw = m_instance->settings()->get(key).toString().trimmed();
    if (raw.isEmpty())
        return {};

    QJsonParseError error{};
    const auto doc = QJsonDocument::fromJson(raw.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) {
        // Fall back to newline / semicolon separated values for resilience.
        return raw.split(QRegularExpression(QStringLiteral("[\n;|]")), Qt::SkipEmptyParts);
    }

    QStringList result;
    for (const auto& value : doc.array()) {
        const auto text = value.toString().trimmed();
        if (!text.isEmpty())
            result << text;
    }
    return result;
}

void DevelopingModWatcher::writeStringListSetting(const QString& key, const QStringList& values)
{
    QJsonArray array;
    for (const auto& value : values)
        array.append(value);
    m_instance->settings()->set(key, QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
}

void DevelopingModWatcher::reloadFromSettings()
{
    auto* settings = m_instance->settings();
    m_enabled = settings->get("DevelopingModEnabled").toBool();
    m_folders = readStringListSetting("DevelopingModFolders");
    m_managedFiles = readStringListSetting("DevelopingModManagedFiles");

    const auto ignoreRaw = settings->get("DevelopingModIgnorePatterns").toString().trimmed();
    if (ignoreRaw.isEmpty()) {
        m_ignorePatterns = defaultIgnorePatterns();
    } else {
        m_ignorePatterns = ignoreRaw.split(QRegularExpression(QStringLiteral("[\n;]")), Qt::SkipEmptyParts);
        for (auto& pattern : m_ignorePatterns)
            pattern = pattern.trimmed();
        m_ignorePatterns.removeAll(QString());
    }

    updateWatches();

    if (m_enabled && !m_folders.isEmpty()) {
        m_lastSeenSources = collectSourceSignatures();
        m_poll.start();
        setStatus(tr("Auto-watching %n folder(s).", "", m_folders.size()));
        scheduleSync();
    } else {
        m_poll.stop();
        m_lastSeenSources.clear();
        if (!m_enabled)
            setStatus(tr("Disabled."));
        else
            setStatus(tr("Enabled, but no folders are configured."));
    }
}

void DevelopingModWatcher::updateWatches()
{
    const auto current = m_watcher.directories() + m_watcher.files();
    if (!current.isEmpty())
        m_watcher.removePaths(current);

    if (!m_enabled)
        return;

    for (const auto& folder : m_folders) {
        QFileInfo info(folder);
        if (!info.exists() || !info.isDir()) {
            qCWarning(developingModLog) << "Developing mod folder does not exist:" << folder;
            continue;
        }
        const auto absolute = QDir::cleanPath(info.absoluteFilePath());
        if (!m_watcher.addPath(absolute))
            qCWarning(developingModLog) << "Failed to watch developing mod folder:" << absolute;

        // Also watch individual JARs: some Windows/Gradle rewrite paths only notify file watches.
        QDir dir(absolute);
        const auto jars = dir.entryInfoList(QStringList{ QStringLiteral("*.jar") }, QDir::Files | QDir::Readable);
        for (const auto& jar : jars) {
            if (shouldIgnore(jar.fileName()))
                continue;
            const auto jarPath = jar.absoluteFilePath();
            if (!m_watcher.files().contains(jarPath) && !m_watcher.addPath(jarPath))
                qCDebug(developingModLog) << "Could not watch developing mod JAR:" << jarPath;
        }
    }
}

bool DevelopingModWatcher::shouldIgnore(const QString& fileName) const
{
    if (!fileName.endsWith(QLatin1String(".jar"), Qt::CaseInsensitive))
        return true;
    if (fileName.endsWith(QLatin1String(".jar.disabled"), Qt::CaseInsensitive))
        return true;

    for (const auto& pattern : m_ignorePatterns) {
        if (QDir::match(pattern, fileName))
            return true;
    }
    return false;
}

DevelopingModWatcher::SourceSignature DevelopingModWatcher::signatureFor(const QFileInfo& info) const
{
    SourceSignature signature;
    signature.size = info.size();
    signature.mtimeMs = info.lastModified().toMSecsSinceEpoch();
    return signature;
}

QStringList DevelopingModWatcher::collectSourceJars() const
{
    QStringList jars;
    QSet<QString> seenNames;

    for (const auto& folder : m_folders) {
        QDir dir(folder);
        if (!dir.exists())
            continue;

        const auto entries = dir.entryInfoList(QStringList{ QStringLiteral("*.jar") }, QDir::Files | QDir::Readable, QDir::Name);
        for (const auto& entry : entries) {
            if (shouldIgnore(entry.fileName()))
                continue;
            if (entry.size() <= 0)
                continue;
            if (seenNames.contains(entry.fileName())) {
                qCWarning(developingModLog) << "Duplicate developing mod JAR name, later folder wins:" << entry.absoluteFilePath();
            }
            seenNames.insert(entry.fileName());
            jars << entry.absoluteFilePath();
        }
    }

    return jars;
}

QHash<QString, DevelopingModWatcher::SourceSignature> DevelopingModWatcher::collectSourceSignatures() const
{
    QHash<QString, SourceSignature> signatures;
    for (const auto& path : collectSourceJars()) {
        QFileInfo info(path);
        if (!info.exists())
            continue;
        signatures.insert(info.absoluteFilePath(), signatureFor(info));
    }
    return signatures;
}

bool DevelopingModWatcher::sourcesStillWriting(const QStringList& sourceJars) const
{
    const auto now = QDateTime::currentDateTime();
    for (const auto& path : sourceJars) {
        QFileInfo info(path);
        if (!info.exists())
            continue;
        // Gradle often rewrites the JAR in place / via rename; wait until mtime settles.
        if (info.lastModified().msecsTo(now) < kWriteSettleMs)
            return true;

        // Zero-length or locked-looking tiny files mid-write.
        if (info.size() <= 0)
            return true;
    }
    return false;
}

void DevelopingModWatcher::directoryChanged(const QString& path)
{
    Q_UNUSED(path);
    if (!m_enabled)
        return;
    // Directory watches can drop after atomic replaces; refresh before syncing.
    updateWatches();
    scheduleSync();
}

void DevelopingModWatcher::fileChanged(const QString& path)
{
    Q_UNUSED(path);
    if (!m_enabled)
        return;
    updateWatches();
    scheduleSync();
}

void DevelopingModWatcher::pollSources()
{
    if (!m_enabled || m_folders.isEmpty())
        return;

    const auto current = collectSourceSignatures();
    if (current != m_lastSeenSources) {
        m_lastSeenSources = current;
        // Watches often break when Gradle deletes/replaces the JAR.
        updateWatches();
        scheduleSync();
    }
}

void DevelopingModWatcher::scheduleSync()
{
    m_debounce.start();
}

void DevelopingModWatcher::syncNow()
{
    m_debounce.stop();
    performSync();
}

bool DevelopingModWatcher::copyJarToMods(const QString& sourcePath, const QString& destFileName, bool* didCopy)
{
    if (didCopy)
        *didCopy = false;

    const auto modsRoot = m_instance->modsRoot();
    FS::ensureFolderPathExists(modsRoot);

    const auto destPath = FS::PathCombine(modsRoot, destFileName);
    const auto disabledPath = destPath + QStringLiteral(".disabled");

    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || sourceInfo.size() <= 0)
        return false;

    const auto sourceSignature = signatureFor(sourceInfo);
    QFileInfo destInfo(destPath);

    // Compare against the last successfully synced SOURCE state, not destination mtime.
    // Destination timestamps are bumped after copy and would hide same-version rebuilds.
    const auto previous = m_lastSyncedSources.value(destFileName);
    if (destInfo.exists() && destInfo.size() == sourceSignature.size && previous == sourceSignature) {
        return true;
    }

    if (QFile::exists(destPath) && !FS::deletePath(destPath)) {
        qCWarning(developingModLog) << "Could not remove previous developing mod copy:" << destPath;
        return false;
    }
    if (QFile::exists(disabledPath) && !FS::deletePath(disabledPath)) {
        qCWarning(developingModLog) << "Could not remove disabled developing mod copy:" << disabledPath;
        return false;
    }

    // Re-stat source right before copy: Gradle may still be replacing the file.
    sourceInfo.refresh();
    if (!sourceInfo.exists() || sourceInfo.size() <= 0)
        return false;

    if (!QFile::copy(sourceInfo.absoluteFilePath(), destPath)) {
        qCWarning(developingModLog) << "Failed to copy developing mod from" << sourcePath << "to" << destPath;
        return false;
    }

    FS::updateTimestamp(destPath);
    m_lastSyncedSources.insert(destFileName, signatureFor(sourceInfo));
    if (didCopy)
        *didCopy = true;
    qCDebug(developingModLog) << "Synced developing mod:" << sourcePath << "->" << destPath;
    return true;
}

bool DevelopingModWatcher::removeManagedMod(const QString& fileName)
{
    const auto modsRoot = m_instance->modsRoot();
    const auto destPath = FS::PathCombine(modsRoot, fileName);
    const auto disabledPath = destPath + QStringLiteral(".disabled");

    bool ok = true;
    if (QFile::exists(destPath) && !FS::deletePath(destPath)) {
        qCWarning(developingModLog) << "Failed to remove stale developing mod:" << destPath;
        ok = false;
    }
    if (QFile::exists(disabledPath) && !FS::deletePath(disabledPath)) {
        qCWarning(developingModLog) << "Failed to remove stale disabled developing mod:" << disabledPath;
        ok = false;
    }
    m_lastSyncedSources.remove(fileName);
    return ok;
}

void DevelopingModWatcher::performSync()
{
    if (m_syncing)
        return;

    if (!m_enabled) {
        setStatus(tr("Disabled."));
        return;
    }

    const auto sourceJars = collectSourceJars();
    if (sourcesStillWriting(sourceJars)) {
        // Retry shortly — avoid copying a half-written Gradle output.
        scheduleSync();
        return;
    }

    m_syncing = true;
    m_lastSeenSources = collectSourceSignatures();

    QStringList activeManaged;
    int updated = 0;
    int unchanged = 0;
    int removed = 0;
    int failed = 0;

    for (const auto& sourcePath : sourceJars) {
        const QFileInfo info(sourcePath);
        bool didCopy = false;
        if (!copyJarToMods(sourcePath, info.fileName(), &didCopy)) {
            failed++;
            continue;
        }
        activeManaged << info.fileName();
        if (didCopy)
            updated++;
        else
            unchanged++;
    }

    for (const auto& previous : m_managedFiles) {
        if (activeManaged.contains(previous))
            continue;
        if (removeManagedMod(previous))
            removed++;
        else
            failed++;
    }

    m_managedFiles = activeManaged;
    writeStringListSetting("DevelopingModManagedFiles", m_managedFiles);

    if (auto* mods = m_instance->loaderModList())
        mods->update();

    // Keep file watches attached after renames/recreates.
    updateWatches();

    const auto stamp = QDateTime::currentDateTime().toString(QLocale().dateTimeFormat(QLocale::ShortFormat));
    if (failed > 0) {
        setStatus(tr("Auto-sync %1 — %2 updated, %3 removed, %4 failed.")
                      .arg(stamp)
                      .arg(updated)
                      .arg(removed)
                      .arg(failed));
    } else if (updated > 0 || removed > 0) {
        setStatus(tr("Auto-sync %1 — %2 updated, %3 removed.").arg(stamp).arg(updated).arg(removed));
    } else {
        setStatus(tr("Auto-watching — %1 mod(s) up to date (%2).").arg(unchanged).arg(stamp));
    }

    m_syncing = false;
}
