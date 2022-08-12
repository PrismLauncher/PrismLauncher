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
#include <QTimer>

#include "FileSystem.h"

#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourceFolderModel.h"

#define EXEC_UPDATE_TASK(EXEC, VERIFY)                                                  \
    QEventLoop loop;                                                                    \
                                                                                        \
    connect(&model, &ResourceFolderModel::updateFinished, &loop, &QEventLoop::quit);    \
                                                                                        \
    QTimer expire_timer;                                                                \
    expire_timer.callOnTimeout(&loop, &QEventLoop::quit);                               \
    expire_timer.setSingleShot(true);                                                   \
    expire_timer.start(4000);                                                           \
                                                                                        \
    VERIFY(EXEC);                                                                       \
    loop.exec();                                                                        \
                                                                                        \
    QVERIFY2(expire_timer.isActive(), "Timer has expired. The update never finished."); \
    expire_timer.stop();                                                                \
                                                                                        \
    disconnect(&model, nullptr, nullptr, nullptr);

class ResourceFolderModelTest : public QObject
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

            QTimer expire_timer;
            expire_timer.callOnTimeout(&loop, &QEventLoop::quit);
            expire_timer.setSingleShot(true);
            expire_timer.start(4000);

            m.installMod(folder);

            loop.exec();

            QVERIFY2(expire_timer.isActive(), "Timer has expired. The update never finished.");
            expire_timer.stop();

            verify(tempDir.path());
        }

        // 2. test with trailing /
        {
            QString folder = source + '/';
            QTemporaryDir tempDir;
            QEventLoop loop;
            ModFolderModel m(tempDir.path(), true);

            connect(&m, &ModFolderModel::updateFinished, &loop, &QEventLoop::quit);

            QTimer expire_timer;
            expire_timer.callOnTimeout(&loop, &QEventLoop::quit);
            expire_timer.setSingleShot(true);
            expire_timer.start(4000);

            m.installMod(folder);

            loop.exec();

            QVERIFY2(expire_timer.isActive(), "Timer has expired. The update never finished.");
            expire_timer.stop();

            verify(tempDir.path());
        }
    }

    void test_addFromWatch()
    {
        QString source = QFINDTESTDATA("testdata");

        ModFolderModel model(source);

        QCOMPARE(model.size(), 0);

        EXEC_UPDATE_TASK(model.startWatching(), )

        for (auto mod : model.allMods())
            qDebug() << mod->name();

        QCOMPARE(model.size(), 2);

        model.stopWatching();

        while (model.hasPendingParseTasks()) {
            QTest::qSleep(20);
            QCoreApplication::processEvents();
        }
    }

    void test_removeResource()
    {
        QString folder_resource = QFINDTESTDATA("testdata/test_folder");
        QString file_mod = QFINDTESTDATA("testdata/supercoolmod.jar");

        QTemporaryDir tmp;

        ResourceFolderModel model(QDir(tmp.path()));

        QCOMPARE(model.size(), 0);

        {
            EXEC_UPDATE_TASK(model.installResource(file_mod), QVERIFY)
        }

        QCOMPARE(model.size(), 1);
        qDebug() << "Added first mod.";

        {
            EXEC_UPDATE_TASK(model.startWatching(), )
        }

        QCOMPARE(model.size(), 1);
        qDebug() << "Started watching the temp folder.";
        
        {
            EXEC_UPDATE_TASK(model.installResource(folder_resource), QVERIFY)
        }

        QCOMPARE(model.size(), 2);
        qDebug() << "Added second mod.";

        {
            EXEC_UPDATE_TASK(model.uninstallResource("supercoolmod.jar"), QVERIFY);
        }

        QCOMPARE(model.size(), 1);
        qDebug() << "Removed first mod.";

        QString mod_file_name {model.at(0).fileinfo().fileName()};
        QVERIFY(!mod_file_name.isEmpty());

        {
            EXEC_UPDATE_TASK(model.uninstallResource(mod_file_name), QVERIFY);
        }

        QCOMPARE(model.size(), 0);
        qDebug() << "Removed second mod.";

        model.stopWatching();

        while (model.hasPendingParseTasks()) {
            QTest::qSleep(20);
            QCoreApplication::processEvents();
        }
    }
};

QTEST_GUILESS_MAIN(ResourceFolderModelTest)

#include "ResourceFolderModel_test.moc"
