// SPDX-License-Identifier: GPL-3.0-only

#include <sys/stat.h>
#include <QFile>
#include <QOperatingSystemVersion>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

QString prepareMacGameMode(const QString& rootDir);

class MacGameModeTest : public QObject {
    Q_OBJECT
   private slots:
    void installAndReuseStub()
    {
        if (QOperatingSystemVersion::current() < QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 14)) {
            QSKIP("Game Mode requires macOS 14 or later");
        }
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        auto expectedPath = directory.filePath("helpers/gamemode");
        auto first = prepareMacGameMode(directory.path());
        QCOMPARE(first, expectedPath);

        struct stat firstStat{};
        QCOMPARE(lstat(QFile::encodeName(first).constData(), &firstStat), 0);
        QVERIFY(S_ISREG(firstStat.st_mode));
        QCOMPARE(firstStat.st_mode & 0777, static_cast<mode_t>(0755));

        QFile installed(first);
        QVERIFY(installed.open(QIODevice::ReadOnly));
        auto bytes = installed.readAll();
        installed.close();
        QVERIFY(bytes.size() > 4);
        QCOMPARE(bytes.left(4), QByteArray("\xca\xfe\xba\xbe", 4));
        QCOMPARE(QProcess::execute("/usr/bin/codesign", { "-v", first }), 0);

        auto second = prepareMacGameMode(directory.path());
        QCOMPARE(second, expectedPath);
        struct stat secondStat{};
        QCOMPARE(lstat(QFile::encodeName(second).constData(), &secondStat), 0);
        QCOMPARE(secondStat.st_ino, firstStat.st_ino);

        QVERIFY(installed.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(installed.write("corrupted") > 0);
        installed.close();

        auto repaired = prepareMacGameMode(directory.path());
        QCOMPARE(repaired, expectedPath);
        QVERIFY(installed.open(QIODevice::ReadOnly));
        QCOMPARE(installed.readAll(), bytes);
        installed.close();

        QVERIFY(prepareMacGameMode({}).isEmpty());
    }
};
QTEST_GUILESS_MAIN(MacGameModeTest)
#include "MacGameMode_test.moc"
