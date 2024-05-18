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

#include <QTest>

#include <FileSystem.h>
#include <RuntimeContext.h>
#include <minecraft/Library.h>
#include <minecraft/MojangVersionFormat.h>
#include <minecraft/OneSixVersionFormat.h>
#include <net/HttpMetaCache.h>

class LibraryTest : public QObject {
    Q_OBJECT
   private:
    LibraryPtr readMojangJson(const QString path)
    {
        QFile jsonFile(path);
        jsonFile.open(QIODevice::ReadOnly);
        auto data = jsonFile.readAll();
        jsonFile.close();
        ProblemContainer problems;
        return MojangVersionFormat::libraryFromJson(problems, QJsonDocument::fromJson(data).object(), path);
    }
    // get absolute path to expected storage, assuming default cache prefix
    QStringList getStorage(QString relative) { return { FS::PathCombine(cache->getBasePath("libraries"), relative) }; }

    RuntimeContext dummyContext(QString system = "linux", QString arch = "64", QString realArch = "amd64")
    {
        RuntimeContext r;
        r.javaArchitecture = arch;
        r.javaRealArchitecture = realArch;
        r.system = system;
        return r;
    }
   private slots:
    void initTestCase()
    {
        cache.reset(new HttpMetaCache());
        cache->addBase("libraries", QDir("libraries").absolutePath());
        dataDir = QDir(QFINDTESTDATA("testdata/Library")).absolutePath();
    }
    void test_legacy()
    {
        RuntimeContext r = dummyContext();
        Library test("test.package:testname:testversion");
        QCOMPARE(test.artifactPrefix(), QString("test.package:testname"));
        QCOMPARE(test.isNative(), false);

        QStringList jar, native, native32, native64;
        test.getApplicableFiles(r, jar, native, native32, native64, QString());
        QCOMPARE(jar, getStorage("test/package/testname/testversion/testname-testversion.jar"));
        QCOMPARE(native, {});
        QCOMPARE(native32, {});
        QCOMPARE(native64, {});
    }
    void test_legacy_url()
    {
        RuntimeContext r = dummyContext();
        QStringList failedFiles;
        Library test("test.package:testname:testversion");
        test.setRepositoryURL("file://foo/bar");
        auto downloads = test.getDownloads(r, cache.get(), failedFiles, QString());
        QCOMPARE(downloads.size(), 1);
        QCOMPARE(failedFiles, {});
        Net::NetRequest::Ptr dl = downloads[0];
        QCOMPARE(dl->url(), QUrl("file://foo/bar/test/package/testname/testversion/testname-testversion.jar"));
    }
    void test_legacy_url_local_broken()
    {
        RuntimeContext r = dummyContext();
        Library test("test.package:testname:testversion");
        QCOMPARE(test.isNative(), false);
        QStringList failedFiles;
        test.setHint("local");
        auto downloads = test.getDownloads(r, cache.get(), failedFiles, QString());
        QCOMPARE(downloads.size(), 0);
        QCOMPARE(failedFiles, { "testname-testversion.jar" });
    }
    void test_legacy_url_local_override()
    {
        RuntimeContext r = dummyContext();
        Library test("com.paulscode:codecwav:20101023");
        QCOMPARE(test.isNative(), false);
        QStringList failedFiles;
        test.setHint("local");
        auto downloads = test.getDownloads(r, cache.get(), failedFiles, QFINDTESTDATA("testdata/Library"));
        QCOMPARE(downloads.size(), 0);
        qDebug() << failedFiles;
        QCOMPARE(failedFiles.size(), 0);

        QStringList jar, native, native32, native64;
        test.getApplicableFiles(r, jar, native, native32, native64, QFINDTESTDATA("testdata/Library"));
        QCOMPARE(jar, { QFileInfo(QFINDTESTDATA("testdata/Library/codecwav-20101023.jar")).absoluteFilePath() });
        QCOMPARE(native, {});
        QCOMPARE(native32, {});
        QCOMPARE(native64, {});
    }
    void test_legacy_native()
    {
        RuntimeContext r = dummyContext();
        Library test("test.package:testname:testversion");
        test.m_nativeClassifiers["linux"] = "linux";
        QCOMPARE(test.isNative(), true);
        test.setRepositoryURL("file://foo/bar");
        {
            QStringList jar, native, native32, native64;
            test.getApplicableFiles(r, jar, native, native32, native64, QString());
            QCOMPARE(jar, {});
            QCOMPARE(native, getStorage("test/package/testname/testversion/testname-testversion-linux.jar"));
            QCOMPARE(native32, {});
            QCOMPARE(native64, {});
            QStringList failedFiles;
            auto dls = test.getDownloads(r, cache.get(), failedFiles, QString());
            QCOMPARE(dls.size(), 1);
            QCOMPARE(failedFiles, {});
            auto dl = dls[0];
            QCOMPARE(dl->url(), QUrl("file://foo/bar/test/package/testname/testversion/testname-testversion-linux.jar"));
        }
    }
    void test_legacy_native_arch()
    {
        RuntimeContext r = dummyContext();
        Library test("test.package:testname:testversion");
        test.m_nativeClassifiers["linux"] = "linux-${arch}";
        test.m_nativeClassifiers["osx"] = "osx-${arch}";
        test.m_nativeClassifiers["windows"] = "windows-${arch}";
        QCOMPARE(test.isNative(), true);
        test.setRepositoryURL("file://foo/bar");
        {
            QStringList jar, native, native32, native64;
            test.getApplicableFiles(r, jar, native, native32, native64, QString());
            QCOMPARE(jar, {});
            QCOMPARE(native, {});
            QCOMPARE(native32, getStorage("test/package/testname/testversion/testname-testversion-linux-32.jar"));
            QCOMPARE(native64, getStorage("test/package/testname/testversion/testname-testversion-linux-64.jar"));
            QStringList failedFiles;
            auto dls = test.getDownloads(r, cache.get(), failedFiles, QString());
            QCOMPARE(dls.size(), 2);
            QCOMPARE(failedFiles, {});
            QCOMPARE(dls[0]->url(), QUrl("file://foo/bar/test/package/testname/testversion/testname-testversion-linux-32.jar"));
            QCOMPARE(dls[1]->url(), QUrl("file://foo/bar/test/package/testname/testversion/testname-testversion-linux-64.jar"));
        }
        r.system = "windows";
        {
            QStringList jar, native, native32, native64;
            test.getApplicableFiles(r, jar, native, native32, native64, QString());
            QCOMPARE(jar, {});
            QCOMPARE(native, {});
            QCOMPARE(native32, getStorage("test/package/testname/testversion/testname-testversion-windows-32.jar"));
            QCOMPARE(native64, getStorage("test/package/testname/testversion/testname-testversion-windows-64.jar"));
            QStringList failedFiles;
            auto dls = test.getDownloads(r, cache.get(), failedFiles, QString());
            QCOMPARE(dls.size(), 2);
            QCOMPARE(failedFiles, {});
            QCOMPARE(dls[0]->url(), QUrl("file://foo/bar/test/package/testname/testversion/testname-testversion-windows-32.jar"));
            QCOMPARE(dls[1]->url(), QUrl("file://foo/bar/test/package/testname/testversion/testname-testversion-windows-64.jar"));
        }
        r.system = "osx";
        {
            QStringList jar, native, native32, native64;
            test.getApplicableFiles(r, jar, native, native32, native64, QString());
            QCOMPARE(jar, {});
            QCOMPARE(native, {});
            QCOMPARE(native32, getStorage("test/package/testname/testversion/testname-testversion-osx-32.jar"));
            QCOMPARE(native64, getStorage("test/package/testname/testversion/testname-testversion-osx-64.jar"));
            QStringList failedFiles;
            auto dls = test.getDownloads(r, cache.get(), failedFiles, QString());
            QCOMPARE(dls.size(), 2);
            QCOMPARE(failedFiles, {});
            QCOMPARE(dls[0]->url(), QUrl("file://foo/bar/test/package/testname/testversion/testname-testversion-osx-32.jar"));
            QCOMPARE(dls[1]->url(), QUrl("file://foo/bar/test/package/testname/testversion/testname-testversion-osx-64.jar"));
        }
    }
    void test_legacy_native_arch_local_override()
    {
        RuntimeContext r = dummyContext();
        Library test("test.package:testname:testversion");
        test.m_nativeClassifiers["linux"] = "linux-${arch}";
        test.setHint("local");
        QCOMPARE(test.isNative(), true);
        test.setRepositoryURL("file://foo/bar");
        {
            QStringList jar, native, native32, native64;
            test.getApplicableFiles(r, jar, native, native32, native64, QFINDTESTDATA("testdata/Library"));
            QCOMPARE(jar, {});
            QCOMPARE(native, {});
            QCOMPARE(native32, { QFileInfo(QFINDTESTDATA("testdata/Library/testname-testversion-linux-32.jar")).absoluteFilePath() });
            QCOMPARE(native64, { QFileInfo(QFINDTESTDATA("testdata/Library") + "/testname-testversion-linux-64.jar").absoluteFilePath() });
            QStringList failedFiles;
            auto dls = test.getDownloads(r, cache.get(), failedFiles, QFINDTESTDATA("testdata/Library"));
            QCOMPARE(dls.size(), 0);
            QCOMPARE(failedFiles,
                     { QFileInfo(QFINDTESTDATA("testdata/Library") + "/testname-testversion-linux-64.jar").absoluteFilePath() });
        }
    }
    void test_onenine()
    {
        RuntimeContext r = dummyContext("osx");
        auto test = readMojangJson(QFINDTESTDATA("testdata/Library/lib-simple.json"));
        {
            QStringList jar, native, native32, native64;
            test->getApplicableFiles(r, jar, native, native32, native64, QString());
            QCOMPARE(jar, getStorage("com/paulscode/codecwav/20101023/codecwav-20101023.jar"));
            QCOMPARE(native, {});
            QCOMPARE(native32, {});
            QCOMPARE(native64, {});
        }
        r.system = "linux";
        {
            QStringList failedFiles;
            auto dls = test->getDownloads(r, cache.get(), failedFiles, QString());
            QCOMPARE(dls.size(), 1);
            QCOMPARE(failedFiles, {});
            QCOMPARE(dls[0]->url(), QUrl("https://libraries.minecraft.net/com/paulscode/codecwav/20101023/codecwav-20101023.jar"));
        }
        r.system = "osx";
        test->setHint("local");
        {
            QStringList jar, native, native32, native64;
            test->getApplicableFiles(r, jar, native, native32, native64, QFINDTESTDATA("testdata/Library"));
            QCOMPARE(jar, { QFileInfo(QFINDTESTDATA("testdata/Library/codecwav-20101023.jar")).absoluteFilePath() });
            QCOMPARE(native, {});
            QCOMPARE(native32, {});
            QCOMPARE(native64, {});
        }
        r.system = "linux";
        {
            QStringList failedFiles;
            auto dls = test->getDownloads(r, cache.get(), failedFiles, QFINDTESTDATA("testdata/Library"));
            QCOMPARE(dls.size(), 0);
            QCOMPARE(failedFiles, {});
        }
    }
    void test_onenine_local_override()
    {
        RuntimeContext r = dummyContext("osx");
        auto test = readMojangJson(QFINDTESTDATA("testdata/Library/lib-simple.json"));
        test->setHint("local");
        {
            QStringList jar, native, native32, native64;
            test->getApplicableFiles(r, jar, native, native32, native64, QFINDTESTDATA("testdata/Library"));
            QCOMPARE(jar, { QFileInfo(QFINDTESTDATA("testdata/Library/codecwav-20101023.jar")).absoluteFilePath() });
            QCOMPARE(native, {});
            QCOMPARE(native32, {});
            QCOMPARE(native64, {});
        }
        r.system = "linux";
        {
            QStringList failedFiles;
            auto dls = test->getDownloads(r, cache.get(), failedFiles, QFINDTESTDATA("testdata/Library"));
            QCOMPARE(dls.size(), 0);
            QCOMPARE(failedFiles, {});
        }
    }
    void test_onenine_native()
    {
        RuntimeContext r = dummyContext("osx");
        auto test = readMojangJson(QFINDTESTDATA("testdata/Library/lib-native.json"));
        QStringList jar, native, native32, native64;
        test->getApplicableFiles(r, jar, native, native32, native64, QString());
        QCOMPARE(jar, QStringList());
        QCOMPARE(native,
                 getStorage("org/lwjgl/lwjgl/lwjgl-platform/2.9.4-nightly-20150209/lwjgl-platform-2.9.4-nightly-20150209-natives-osx.jar"));
        QCOMPARE(native32, {});
        QCOMPARE(native64, {});
        QStringList failedFiles;
        auto dls = test->getDownloads(r, cache.get(), failedFiles, QString());
        QCOMPARE(dls.size(), 1);
        QCOMPARE(failedFiles, {});
        QCOMPARE(dls[0]->url(), QUrl("https://libraries.minecraft.net/org/lwjgl/lwjgl/lwjgl-platform/2.9.4-nightly-20150209/"
                                     "lwjgl-platform-2.9.4-nightly-20150209-natives-osx.jar"));
    }
    void test_onenine_native_arch()
    {
        RuntimeContext r = dummyContext("windows");
        auto test = readMojangJson(QFINDTESTDATA("testdata/Library/lib-native-arch.json"));
        QStringList jar, native, native32, native64;
        test->getApplicableFiles(r, jar, native, native32, native64, QString());
        QCOMPARE(jar, {});
        QCOMPARE(native, {});
        QCOMPARE(native32, getStorage("tv/twitch/twitch-platform/5.16/twitch-platform-5.16-natives-windows-32.jar"));
        QCOMPARE(native64, getStorage("tv/twitch/twitch-platform/5.16/twitch-platform-5.16-natives-windows-64.jar"));
        QStringList failedFiles;
        auto dls = test->getDownloads(r, cache.get(), failedFiles, QString());
        QCOMPARE(dls.size(), 2);
        QCOMPARE(failedFiles, {});
        QCOMPARE(dls[0]->url(),
                 QUrl("https://libraries.minecraft.net/tv/twitch/twitch-platform/5.16/twitch-platform-5.16-natives-windows-32.jar"));
        QCOMPARE(dls[1]->url(),
                 QUrl("https://libraries.minecraft.net/tv/twitch/twitch-platform/5.16/twitch-platform-5.16-natives-windows-64.jar"));
    }

   private:
    std::unique_ptr<HttpMetaCache> cache;
    QString dataDir;
};

QTEST_GUILESS_MAIN(LibraryTest)

#include "Library_test.moc"
