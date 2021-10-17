#include <QTest>
#include <QSignalSpy>

#include "TestUtil.h"

#include "updater/GoUpdate.h"
#include "updater/DownloadTask.h"
#include "updater/UpdateChecker.h"
#include <FileSystem.h>

using namespace GoUpdate;

FileSourceList encodeBaseFile(const char *suffix)
{
    auto base = QDir::currentPath();
    QUrl localFile = QUrl::fromLocalFile(base + suffix);
    QString localUrlString = localFile.toString(QUrl::FullyEncoded);
    auto item = FileSource("http", localUrlString);
    return FileSourceList({item});
}

Q_DECLARE_METATYPE(VersionFileList)
Q_DECLARE_METATYPE(Operation)

QDebug operator<<(QDebug dbg, const FileSource &f)
{
    dbg.nospace() << "FileSource(type=" << f.type << " url=" << f.url
                  << " comp=" << f.compressionType << ")";
    return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const VersionFileEntry &v)
{
    dbg.nospace() << "VersionFileEntry(path=" << v.path << " mode=" << v.mode
                  << " md5=" << v.md5 << " sources=" << v.sources << ")";
    return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const Operation::Type &t)
{
    switch (t)
    {
    case Operation::OP_REPLACE:
        dbg << "OP_COPY";
        break;
    case Operation::OP_DELETE:
        dbg << "OP_DELETE";
        break;
    }
    return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const Operation &u)
{
    dbg.nospace() << "Operation(type=" << u.type << " file=" << u.source
                  << " dest=" << u.destination << " mode=" << u.destinationMode << ")";
    return dbg.maybeSpace();
}

class DownloadTaskTest : public QObject
{
    Q_OBJECT
private
slots:
    void initTestCase()
    {
    }
    void cleanupTestCase()
    {
    }

    void test_parseVersionInfo_data()
    {
        QTest::addColumn<QByteArray>("data");
        QTest::addColumn<VersionFileList>("list");
        QTest::addColumn<QString>("error");
        QTest::addColumn<bool>("ret");

        QTest::newRow("one")
            << GET_TEST_FILE("data/1.json")
            << (VersionFileList()
                << VersionFileEntry{"fileOne",
                                                        493,
                                                        encodeBaseFile("/data/fileOneA"),
                                                        "9eb84090956c484e32cb6c08455a667b"}
                << VersionFileEntry{"fileTwo",
                                                        644,
                                                        encodeBaseFile("/data/fileTwo"),
                                                        "38f94f54fa3eb72b0ea836538c10b043"}
                << VersionFileEntry{"fileThree",
                                                        750,
                                                        encodeBaseFile("/data/fileThree"),
                                                        "f12df554b21e320be6471d7154130e70"})
            << QString() << true;
        QTest::newRow("two")
            << GET_TEST_FILE("data/2.json")
            << (VersionFileList()
                << VersionFileEntry{"fileOne",
                                                        493,
                                                        encodeBaseFile("/data/fileOneB"),
                                                        "42915a71277c9016668cce7b82c6b577"}
                << VersionFileEntry{"fileTwo",
                                                        644,
                                                        encodeBaseFile("/data/fileTwo"),
                                                        "38f94f54fa3eb72b0ea836538c10b043"})
            << QString() << true;
    }
    void test_parseVersionInfo()
    {
        QFETCH(QByteArray, data);
        QFETCH(VersionFileList, list);
        QFETCH(QString, error);
        QFETCH(bool, ret);

        VersionFileList outList;
        QString outError;
        bool outRet = parseVersionInfo(data, outList, outError);
        QCOMPARE(outRet, ret);
        QCOMPARE(outList, list);
        QCOMPARE(outError, error);
    }

    void test_processFileLists_data()
    {
        QTest::addColumn<QString>("tempFolder");
        QTest::addColumn<VersionFileList>("currentVersion");
        QTest::addColumn<VersionFileList>("newVersion");
        QTest::addColumn<OperationList>("expectedOperations");

        QTemporaryDir tempFolderObj;
        QString tempFolder = tempFolderObj.path();
        // update fileOne, keep fileTwo, remove fileThree
        QTest::newRow("test 1")
            << tempFolder << (VersionFileList()
                              << VersionFileEntry{
                                     "data/fileOne", 493,
                                     FileSourceList()
                                         << FileSource(
                                                "http", "http://host/path/fileOne-1"),
                                     "9eb84090956c484e32cb6c08455a667b"}
                              << VersionFileEntry{
                                     "data/fileTwo", 644,
                                     FileSourceList()
                                         << FileSource(
                                                "http", "http://host/path/fileTwo-1"),
                                     "38f94f54fa3eb72b0ea836538c10b043"}
                              << VersionFileEntry{
                                     "data/fileThree", 420,
                                     FileSourceList()
                                         << FileSource(
                                                "http", "http://host/path/fileThree-1"),
                                     "f12df554b21e320be6471d7154130e70"})
            << (VersionFileList()
                << VersionFileEntry{
                       "data/fileOne", 493,
                       FileSourceList()
                           << FileSource("http",
                                                             "http://host/path/fileOne-2"),
                       "42915a71277c9016668cce7b82c6b577"}
                << VersionFileEntry{
                       "data/fileTwo", 644,
                       FileSourceList()
                           << FileSource("http",
                                                             "http://host/path/fileTwo-2"),
                       "38f94f54fa3eb72b0ea836538c10b043"})
            << (OperationList()
                << Operation::DeleteOp("data/fileThree")
                << Operation::CopyOp(
                       FS::PathCombine(tempFolder,
                                   QString("data/fileOne").replace("/", "_")),
                       "data/fileOne", 493));
    }
    void test_processFileLists()
    {
        QFETCH(QString, tempFolder);
        QFETCH(VersionFileList, currentVersion);
        QFETCH(VersionFileList, newVersion);
        QFETCH(OperationList, expectedOperations);

        OperationList operations;

        processFileLists(currentVersion, newVersion, QDir::currentPath(), tempFolder, new NetJob("Dummy"), operations);
        qDebug() << (operations == expectedOperations);
        qDebug() << operations;
        qDebug() << expectedOperations;
        QCOMPARE(operations, expectedOperations);
    }
};

extern "C"
{
    QTEST_GUILESS_MAIN(DownloadTaskTest)
}

#include "DownloadTask_test.moc"
