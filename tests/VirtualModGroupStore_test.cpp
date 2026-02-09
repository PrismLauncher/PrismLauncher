// SPDX-FileCopyrightText: 2026 abhicommands <114682464+abhicommands@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 abhicommands <114682464+abhicommands@users.noreply.github.com>
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

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "FileSystem.h"
#include "minecraft/mod/VirtualModGroupStore.h"

class VirtualModGroupStoreTest : public QObject {
    Q_OBJECT

   private slots:
    void readWriteRoundTrip()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        auto modsPath = tmp.filePath("minecraft/mods");
        auto indexPath = tmp.filePath("minecraft/mods/.index");
        QVERIFY(FS::ensureFolderPathExists(modsPath));
        QVERIFY(FS::ensureFolderPathExists(indexPath));

        VirtualModGroupStore store{ QDir(modsPath), QDir(indexPath) };
        QVERIFY(store.loadOrCreate());

        auto rootGroupId = store.createGroup("Pack Mods");
        QVERIFY(!rootGroupId.isEmpty());
        auto secondGroupId = store.createGroup("Client", rootGroupId);
        QVERIFY(!secondGroupId.isEmpty());

        VirtualModGroupStore::Entry entry;
        entry.fileKey = "example-mod.jar";
        entry.fileName = "example-mod.jar";
        entry.groupId = secondGroupId;
        entry.sourceType = VirtualModGroupStore::SourceType::MANAGED_PACK;
        store.upsertEntry(entry);

        store.setLegacyNestedFolderMigrationDone(true);
        QVERIFY(store.save());

        VirtualModGroupStore loadedStore{ QDir(modsPath), QDir(indexPath) };
        QVERIFY(loadedStore.load());
        QVERIFY(loadedStore.legacyNestedFolderMigrationDone());
        QVERIFY(loadedStore.hasEntry("example-mod.jar"));
        QVERIFY(loadedStore.isEntryInGroupSubtree("example-mod.jar", secondGroupId));
        QVERIFY(!loadedStore.isEntryInGroupSubtree("example-mod.jar", rootGroupId));

        for (auto const& group : loadedStore.groups()) {
            QVERIFY(group.parentId.isEmpty());
        }
    }

    void invalidJsonCanBeRecovered()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        auto modsPath = tmp.filePath("minecraft/mods");
        auto indexPath = tmp.filePath("minecraft/mods/.index");
        QVERIFY(FS::ensureFolderPathExists(modsPath));
        QVERIFY(FS::ensureFolderPathExists(indexPath));

        auto storePath = QDir(indexPath).absoluteFilePath(VirtualModGroupStore::STORE_FILE_NAME);
        QFile file(storePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("{ this is not json");
        file.close();

        VirtualModGroupStore store{ QDir(modsPath), QDir(indexPath) };
        QVERIFY(!store.load());
        QVERIFY(store.loadOrCreate());
        QVERIFY(store.save());
    }
};

QTEST_GUILESS_MAIN(VirtualModGroupStoreTest)

#include "VirtualModGroupStore_test.moc"
