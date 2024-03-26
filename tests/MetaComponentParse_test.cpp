// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTest>
#include <QTimer>

#include <FileSystem.h>

#include <minecraft/mod/tasks/LocalResourcePackParseTask.h>

class MetaComponentParseTest : public QObject {
    Q_OBJECT

    void doTest(QString name)
    {
        QString source = QFINDTESTDATA("testdata/MetaComponentParse");

        QString comp_rp = FS::PathCombine(source, name);

        QFile file;
        file.setFileName(comp_rp);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonObject obj = doc.object();

        QJsonValue description_json = obj.value("description");
        QJsonValue expected_json = obj.value("expected_output");

        QVERIFY(!description_json.isUndefined());
        QVERIFY(expected_json.isString());

        QString expected = expected_json.toString();

        QString processed = ResourcePackUtils::processComponent(description_json);

        QCOMPARE(processed, expected);
    }

   private slots:
    void test_parseComponentBasic() { doTest("component_basic.json"); }
    void test_parseComponentWithFormat() { doTest("component_with_format.json"); }
    void test_parseComponentWithExtra() { doTest("component_with_extra.json"); }
    void test_parseComponentWithLink() { doTest("component_with_link.json"); }
    void test_parseComponentWithMixed() { doTest("component_with_mixed.json"); }
};

QTEST_GUILESS_MAIN(MetaComponentParseTest)

#include "MetaComponentParse_test.moc"
