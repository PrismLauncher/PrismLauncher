// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
* This file incorporates work covered by the following copyright and
* permission notice:
*
*      Copyright 2013-2021 MultiMC Contributors
*
*      Licensed under the Apache License, Version 2.0 (the "License");
*      you may not use this file except in compliance with the License.
*      You may obtain a copy of the License at
*
*          http://www.apache.org/licenses/LICENSE-2.0
*
*      Unless required by applicable law or agreed to in writing, software
*      distributed under the License is distributed on an "AS IS" BASIS,
*      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*      See the License for the specific language governing permissions and
*      limitations under the License.
*/

#include <QTest>
#include <QTemporaryDir>

#include "FileSystem.h"
#include "minecraft/mod/ModFolderModel.h"

class ModFolderModelTest : public QObject
{
    Q_OBJECT

private
slots:
    // test for GH-1178 - install a folder with files to a mod list
    void test_1178()
    {
        // source
        QString source = QFINDTESTDATA("testdata/test_folder");

        // sanity check
        QVERIFY(!source.endsWith('/'));

        auto verify = [](QString path)
        {
            QDir target_dir(FS::PathCombine(path, "test_folder"));
            QVERIFY(target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(target_dir.entryList().contains("assets"));
        };

        // 1. test with no trailing /
        {
            QString folder = source;
            QTemporaryDir tempDir;
            QEventLoop loop;
            ModFolderModel m(tempDir.path(), true);
            connect(&m, &ModFolderModel::updateFinished, &loop, &QEventLoop::quit);
            m.installMod(folder);
            loop.exec();
            verify(tempDir.path());
        }

        // 2. test with trailing /
        {
            QString folder = source + '/';
            QTemporaryDir tempDir;
            QEventLoop loop;
            ModFolderModel m(tempDir.path(), true);
            connect(&m, &ModFolderModel::updateFinished, &loop, &QEventLoop::quit);
            m.installMod(folder);
            loop.exec();
            verify(tempDir.path());
        }
    }
};

QTEST_GUILESS_MAIN(ModFolderModelTest)

#include "ModFolderModel_test.moc"
