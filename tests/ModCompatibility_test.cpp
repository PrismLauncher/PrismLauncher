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

#include "minecraft/mod/ModCompatibility.h"
#include "minecraft/mod/Resource.h"

class ModCompatibilityTest : public QObject {
    Q_OBJECT

   private slots:
    void supportsVersionPatterns()
    {
        QVERIFY(ModCompatibility::supportsMinecraftVersion({ "1.21.1" }, "1.21.1"));
        QVERIFY(ModCompatibility::supportsMinecraftVersion({ "1.21.x" }, "1.21.11"));
        QVERIFY(ModCompatibility::supportsMinecraftVersion({ "*" }, "1.20.4"));
        QVERIFY(!ModCompatibility::supportsMinecraftVersion({ "1.20.6" }, "1.21.1"));
    }

    void incompatibleEnabledResourceIsReported()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        auto enabledFilePath = tmp.filePath("example.jar");
        QFile enabledFile(enabledFilePath);
        QVERIFY(enabledFile.open(QIODevice::WriteOnly));
        enabledFile.close();

        Resource resource{ QFileInfo(enabledFilePath) };

        Metadata::ModStruct metadata;
        metadata.mcVersions = { "1.20.6" };
        resource.setMetadata(metadata);

        QVERIFY(ModCompatibility::isIncompatibleWithInstanceVersion(resource, "1.21.1"));
    }

    void disabledResourceDoesNotReportIncompatibility()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        auto disabledFilePath = tmp.filePath("example.jar.disabled");
        QFile disabledFile(disabledFilePath);
        QVERIFY(disabledFile.open(QIODevice::WriteOnly));
        disabledFile.close();

        Resource resource{ QFileInfo(disabledFilePath) };

        Metadata::ModStruct metadata;
        metadata.mcVersions = { "1.20.6" };
        resource.setMetadata(metadata);

        QVERIFY(!ModCompatibility::isIncompatibleWithInstanceVersion(resource, "1.21.1"));
    }
};

QTEST_GUILESS_MAIN(ModCompatibilityTest)

#include "ModCompatibility_test.moc"
