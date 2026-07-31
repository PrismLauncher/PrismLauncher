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

#pragma once

#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>

class MinecraftInstance;

/**
 * Watches Gradle/Java build output folders and copies matching JAR files into
 * the instance mods folder, replacing previous developing-mod copies.
 *
 * Uses QFileSystemWatcher plus a polling timer so Windows/Gradle atomic
 * rewrites (same filename/version) are still picked up automatically.
 */
class DevelopingModWatcher : public QObject {
    Q_OBJECT

   public:
    static QStringList defaultIgnorePatterns();

    explicit DevelopingModWatcher(MinecraftInstance* instance);

    void reloadFromSettings();
    void syncNow();

    bool isEnabled() const { return m_enabled; }
    QString statusText() const { return m_statusText; }

   signals:
    void statusChanged(const QString& status);

   private slots:
    void directoryChanged(const QString& path);
    void fileChanged(const QString& path);
    void pollSources();
    void performSync();

   private:
    struct SourceSignature {
        qint64 size = -1;
        qint64 mtimeMs = -1;

        bool operator==(const SourceSignature& other) const { return size == other.size && mtimeMs == other.mtimeMs; }
        bool operator!=(const SourceSignature& other) const { return !(*this == other); }
    };

    void setStatus(const QString& status);
    void scheduleSync();
    void updateWatches();
    bool shouldIgnore(const QString& fileName) const;
    QStringList collectSourceJars() const;
    QHash<QString, SourceSignature> collectSourceSignatures() const;
    SourceSignature signatureFor(const QFileInfo& info) const;
    bool sourcesStillWriting(const QStringList& sourceJars) const;
    QStringList readStringListSetting(const QString& key) const;
    void writeStringListSetting(const QString& key, const QStringList& values);
    bool copyJarToMods(const QString& sourcePath, const QString& destFileName, bool* didCopy = nullptr);
    bool removeManagedMod(const QString& fileName);

    MinecraftInstance* m_instance = nullptr;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QTimer m_poll;
    bool m_enabled = false;
    QStringList m_folders;
    QStringList m_ignorePatterns;
    QStringList m_managedFiles;
    QHash<QString, SourceSignature> m_lastSyncedSources;  // dest file name -> last copied source signature
    QHash<QString, SourceSignature> m_lastSeenSources;    // absolute source path -> last observed signature
    QString m_statusText;
    bool m_syncing = false;
};
