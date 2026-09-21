// SPDX-License-Identifier: GPL-3.0-only

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <shared/SharedOptionsFile.h>

class SharedOptionsFileTest : public QObject {
    Q_OBJECT

   private slots:
    void overlayPreservesTargetStructure()
    {
        const QByteArray shared = "music:0.25\nserver:example.org:25565\nnewSetting:on\n";
        const QByteArray target = "# malformed\nmusic:1.0\nserver:localhost:25565\nlocalOnly:value\nbroken\n";

        QCOMPARE(SharedOptionsFile::overlay(shared, target),
                 QByteArray("# malformed\nmusic:0.25\nserver:example.org:25565\nlocalOnly:value\nbroken\nnewSetting:on\n"));
    }

    void overlayHonorsExclusions()
    {
        const QSet<QString> excluded{ "music", "newSetting" };
        QCOMPARE(SharedOptionsFile::overlay("music:0.25\nnewSetting:on\ngamma:1.0\n", "music:1.0\ngamma:0.5\n", excluded),
                 QByteArray("music:1.0\ngamma:1.0\n"));
    }

    void overlayInitializesAbsentOrEmptyTarget()
    {
        const QByteArray shared = "music:0.25\ncustomMod:key:value\nmalformed\n";
        QCOMPARE(SharedOptionsFile::overlay(shared, {}), shared);
        QCOMPARE(SharedOptionsFile::overlay(shared, {}, { "customMod" }), QByteArray("music:0.25\nmalformed\n"));
    }

    void mergeIsAUnion()
    {
        const QByteArray shared = "sharedOnly:yes\nmusic:1.0\nmalformed shared\n";
        const QByteArray local = "music:0.25\nlocalOnly:a:b:c\nmalformed local\n";
        QCOMPARE(SharedOptionsFile::mergeIntoShared(shared, local),
                 QByteArray("sharedOnly:yes\nmusic:0.25\nmalformed shared\nlocalOnly:a:b:c\n"));
    }

    void mergeHonorsExclusions()
    {
        const QSet<QString> excluded{ "music", "localOnly" };
        QCOMPARE(SharedOptionsFile::mergeIntoShared("music:1.0\nsharedOnly:yes\n", "music:0.25\nlocalOnly:no\n", excluded),
                 QByteArray("music:1.0\nsharedOnly:yes\n"));
    }

    void preservesCrlfAndNoFinalNewline()
    {
        QCOMPARE(SharedOptionsFile::overlay("music:0.25\r\n", "music:1.0\r\nbroken"), QByteArray("music:0.25\r\nbroken"));
        QCOMPARE(SharedOptionsFile::mergeIntoShared("music:1.0\r\nlast:value", "added:a:b\r\n"),
                 QByteArray("music:1.0\r\nlast:value\r\nadded:a:b\r\n"));
    }

    void fileOperationsAreAtomicAndSupportMissingTargets()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        const auto sharedPath = temporaryDir.filePath("group/options.txt");
        const auto localPath = temporaryDir.filePath("instance/options.txt");

        QVERIFY(write(sharedPath, "music:0.25\ncustom:value\n"));
        QString error;
        QVERIFY2(SharedOptionsFile::overlayFile(sharedPath, localPath, {}, &error), qPrintable(error));
        QCOMPARE(read(localPath), QByteArray("music:0.25\ncustom:value\n"));

        QVERIFY(write(localPath, "music:0.75\ncustom:value\nnewKey:a:b\n"));
        QVERIFY2(SharedOptionsFile::updateSharedFile(localPath, sharedPath, { "custom" }, &error), qPrintable(error));
        QCOMPARE(read(sharedPath), QByteArray("music:0.75\ncustom:value\nnewKey:a:b\n"));
    }

   private:
    static bool write(const QString& path, const QByteArray& contents)
    {
        QFileInfo info(path);
        if (!QDir().mkpath(info.absolutePath())) {
            return false;
        }
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    static QByteArray read(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }
};

QTEST_GUILESS_MAIN(SharedOptionsFileTest)

#include "SharedOptionsFile_test.moc"
