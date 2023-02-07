#include <QTest>
#include <QTemporaryDir>
#include <QStandardPaths>

#include <FileSystem.h>

#include <pathmatcher/RegexpMatcher.h>

class FileSystemTest : public QObject
{
    Q_OBJECT

    const QString bothSlash = "/foo/";
    const QString trailingSlash = "foo/";
    const QString leadingSlash = "/foo";

private
slots:
    void test_pathCombine()
    {
        QCOMPARE(QString("/foo/foo"), FS::PathCombine(bothSlash, bothSlash));
        QCOMPARE(QString("foo/foo"), FS::PathCombine(trailingSlash, trailingSlash));
        QCOMPARE(QString("/foo/foo"), FS::PathCombine(leadingSlash, leadingSlash));

        QCOMPARE(QString("/foo/foo/foo"), FS::PathCombine(bothSlash, bothSlash, bothSlash));
        QCOMPARE(QString("foo/foo/foo"), FS::PathCombine(trailingSlash, trailingSlash, trailingSlash));
        QCOMPARE(QString("/foo/foo/foo"), FS::PathCombine(leadingSlash, leadingSlash, leadingSlash));
    }

    void test_PathCombine1_data()
    {
        QTest::addColumn<QString>("result");
        QTest::addColumn<QString>("path1");
        QTest::addColumn<QString>("path2");

        QTest::newRow("qt 1") << "/abc/def/ghi/jkl" << "/abc/def" << "ghi/jkl";
        QTest::newRow("qt 2") << "/abc/def/ghi/jkl" << "/abc/def/" << "ghi/jkl";
#if defined(Q_OS_WIN)
        QTest::newRow("win native, from C:") << "C:/abc" << "C:" << "abc";
        QTest::newRow("win native 1") << "C:/abc/def/ghi/jkl" << "C:\\abc\\def" << "ghi\\jkl";
        QTest::newRow("win native 2") << "C:/abc/def/ghi/jkl" << "C:\\abc\\def\\" << "ghi\\jkl";
#endif
    }

    void test_PathCombine1()
    {
        QFETCH(QString, result);
        QFETCH(QString, path1);
        QFETCH(QString, path2);

        QCOMPARE(FS::PathCombine(path1, path2), result);
    }

    void test_PathCombine2_data()
    {
        QTest::addColumn<QString>("result");
        QTest::addColumn<QString>("path1");
        QTest::addColumn<QString>("path2");
        QTest::addColumn<QString>("path3");

        QTest::newRow("qt 1") << "/abc/def/ghi/jkl" << "/abc" << "def" << "ghi/jkl";
        QTest::newRow("qt 2") << "/abc/def/ghi/jkl" << "/abc/" << "def" << "ghi/jkl";
        QTest::newRow("qt 3") << "/abc/def/ghi/jkl" << "/abc" << "def/" << "ghi/jkl";
        QTest::newRow("qt 4") << "/abc/def/ghi/jkl" << "/abc/" << "def/" << "ghi/jkl";
#if defined(Q_OS_WIN)
        QTest::newRow("win 1") << "C:/abc/def/ghi/jkl" << "C:\\abc" << "def" << "ghi\\jkl";
        QTest::newRow("win 2") << "C:/abc/def/ghi/jkl" << "C:\\abc\\" << "def" << "ghi\\jkl";
        QTest::newRow("win 3") << "C:/abc/def/ghi/jkl" << "C:\\abc" << "def\\" << "ghi\\jkl";
        QTest::newRow("win 4") << "C:/abc/def/ghi/jkl" << "C:\\abc\\" << "def" << "ghi\\jkl";
#endif
    }

    void test_PathCombine2()
    {
        QFETCH(QString, result);
        QFETCH(QString, path1);
        QFETCH(QString, path2);
        QFETCH(QString, path3);

        QCOMPARE(FS::PathCombine(path1, path2, path3), result);
    }

    void test_copy()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::copy c(folder, target_dir.path());
            c();

            for(auto entry: target_dir.entryList())
            {
                qDebug() << entry;
            }
            QVERIFY(target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(target_dir.entryList().contains("assets"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_copy_with_blacklist()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::copy c(folder, target_dir.path());
            c.matcher(new RegexpMatcher("[.]?mcmeta"));
            c();

            for(auto entry: target_dir.entryList())
            {
                qDebug() << entry;
            }
            QVERIFY(!target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(target_dir.entryList().contains("assets"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_copy_with_whitelist()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::copy c(folder, target_dir.path());
            c.matcher(new RegexpMatcher("[.]?mcmeta"));
            c.whitelist(true);
            c();

            for(auto entry: target_dir.entryList())
            {
                qDebug() << entry;
            }
            QVERIFY(target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(!target_dir.entryList().contains("assets"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_copy_with_dot_hidden()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::copy c(folder, target_dir.path());
            c();

            auto filter = QDir::Filter::Files | QDir::Filter::Dirs | QDir::Filter::Hidden;

            for (auto entry: target_dir.entryList(filter)) {
                qDebug() << entry;
            }

            QVERIFY(target_dir.entryList(filter).contains(".secret_folder"));
            target_dir.cd(".secret_folder");
            QVERIFY(target_dir.entryList(filter).contains(".secret_file.txt"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_copy_single_file()
    {
        QTemporaryDir tempDir;
        tempDir.setAutoRemove(true);

        {
            QString file = QFINDTESTDATA("testdata/FileSystem/test_folder/pack.mcmeta");

            qDebug() << "From:" << file << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "pack.mcmeta"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::copy c(file, target_dir.filePath("pack.mcmeta"));
            c();

            auto filter = QDir::Filter::Files;

            for (auto entry: target_dir.entryList(filter)) {
                qDebug() << entry;
            }

            QVERIFY(target_dir.entryList(filter).contains("pack.mcmeta"));
        }
    }

    void test_getDesktop()
    {
        QCOMPARE(FS::getDesktopDir(), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    }


    void test_link()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::create_link lnk(folder, target_dir.path());
            lnk.linkRecursively(false);
            lnk.debug(true);
            if(!lnk()){
#if defined Q_OS_WIN32
                qDebug() << "EXPECTED: Link failure, Windows requires permissions for symlinks";
                QVERIFY(lnk.getLastOSError() == 1314);
                return;
#endif
                qDebug() << "Link Failed!" << lnk.getLastOSError();
            }

            for(auto entry: target_dir.entryList())
            {
                qDebug() << entry;
                QFileInfo entry_lnk_info(target_dir.filePath(entry));
                QVERIFY(!entry_lnk_info.isSymbolicLink());
            }

            QFileInfo lnk_info(target_dir.path());
            QVERIFY(lnk_info.exists());
            QVERIFY(lnk_info.isSymbolicLink());

            QVERIFY(target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(target_dir.entryList().contains("assets"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_hard_link()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::create_link lnk(folder, target_dir.path());
            lnk.useHardLinks(true);
            lnk.debug(true);
            if(!lnk()){
                qDebug() << "Link Failed!" << lnk.getLastOSError();
            }

            for(auto entry: target_dir.entryList())
            {
                qDebug() << entry;
                QFileInfo entry_lnk_info(target_dir.filePath(entry));
                QVERIFY(!entry_lnk_info.isSymbolicLink());
                QFileInfo entry_orig_info(QDir(folder).filePath(entry));
                if (!entry_lnk_info.isDir()) {
                    qDebug() << "hard link equivalency?" << entry_lnk_info.absoluteFilePath() << "vs" << entry_orig_info.absoluteFilePath();
                    QVERIFY(std::filesystem::equivalent(entry_lnk_info.filesystemAbsoluteFilePath(), entry_orig_info.filesystemAbsoluteFilePath()));
                } 
            }

            QFileInfo lnk_info(target_dir.path());
            QVERIFY(lnk_info.exists());
            QVERIFY(!lnk_info.isSymbolicLink());

            QVERIFY(target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(target_dir.entryList().contains("assets"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_link_with_blacklist()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::create_link lnk(folder, target_dir.path());
            lnk.matcher(new RegexpMatcher("[.]?mcmeta"));
            lnk.linkRecursively(true);
            lnk.debug(true);
            if(!lnk()){
#if defined Q_OS_WIN32
                qDebug() << "EXPECTED: Link failure, Windows requires permissions for symlinks";
                QVERIFY(lnk.getLastOSError() == 1314);
                return;
#endif
                qDebug() << "Link Failed!" << lnk.getLastOSError();
            }

            for(auto entry: target_dir.entryList())
            {
                qDebug() << entry;
                QFileInfo entry_lnk_info(target_dir.filePath(entry));
                QVERIFY(entry_lnk_info.isSymbolicLink());
            }

            QFileInfo lnk_info(target_dir.path());
            QVERIFY(lnk_info.exists());
            QVERIFY(lnk_info.isSymbolicLink());

            QVERIFY(!target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(target_dir.entryList().contains("assets"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_link_with_whitelist()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::create_link lnk(folder, target_dir.path());
            lnk.matcher(new RegexpMatcher("[.]?mcmeta"));
            lnk.whitelist(true);
            lnk.linkRecursively(true);
            lnk.debug(true);
            if(!lnk()){
#if defined Q_OS_WIN32
                qDebug() << "EXPECTED: Link failure, Windows requires permissions for symlinks";
                QVERIFY(lnk.getLastOSError() == 1314);
                return;
#endif
                qDebug() << "Link Failed!" << lnk.getLastOSError();
            }

            for(auto entry: target_dir.entryList())
            {
                qDebug() << entry;
                QFileInfo entry_lnk_info(target_dir.filePath(entry));
                QVERIFY(entry_lnk_info.isSymbolicLink());
            }

            QFileInfo lnk_info(target_dir.path());
            QVERIFY(lnk_info.exists());
            QVERIFY(lnk_info.isSymbolicLink());

            QVERIFY(target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(!target_dir.entryList().contains("assets"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_link_with_dot_hidden()
    {
        QString folder = QFINDTESTDATA("testdata/FileSystem/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::create_link lnk(folder, target_dir.path());
            lnk.linkRecursively(true);
            lnk.debug(true);
            if(!lnk()){
#if defined Q_OS_WIN32
                qDebug() << "EXPECTED: Link failure, Windows requires permissions for symlinks";
                QVERIFY(lnk.getLastOSError() == 1314);
                return;
#endif
                qDebug() << "Link Failed!" << lnk.getLastOSError();
            }

            auto filter = QDir::Filter::Files | QDir::Filter::Dirs | QDir::Filter::Hidden;

            for (auto entry: target_dir.entryList(filter)) {
                qDebug() << entry;
                QFileInfo entry_lnk_info(target_dir.filePath(entry));
                QVERIFY(entry_lnk_info.isSymbolicLink());
            }

            QFileInfo lnk_info(target_dir.path());
            QVERIFY(lnk_info.exists());
            QVERIFY(lnk_info.isSymbolicLink());

            QVERIFY(target_dir.entryList(filter).contains(".secret_folder"));
            target_dir.cd(".secret_folder");
            QVERIFY(target_dir.entryList(filter).contains(".secret_file.txt"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_link_single_file()
    {
        QTemporaryDir tempDir;
        tempDir.setAutoRemove(true);

        {
            QString file = QFINDTESTDATA("testdata/FileSystem/test_folder/pack.mcmeta");

            qDebug() << "From:" << file << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "pack.mcmeta"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::create_link lnk(file, target_dir.filePath("pack.mcmeta"));
            lnk.debug(true);
            if(!lnk()){
#if defined Q_OS_WIN32
                qDebug() << "EXPECTED: Link failure, Windows requires permissions for symlinks";
                QVERIFY(lnk.getLastOSError() == 1314);
                return;
#endif
                qDebug() << "Link Failed!" << lnk.getLastOSError();
            }

            auto filter = QDir::Filter::Files;

            for (auto entry: target_dir.entryList(filter)) {
                qDebug() << entry;
            }

            QFileInfo lnk_info(target_dir.filePath("pack.mcmeta"));
            QVERIFY(lnk_info.exists());
            QVERIFY(lnk_info.isSymbolicLink());

            QVERIFY(target_dir.entryList(filter).contains("pack.mcmeta"));
        }
    }
};

QTEST_GUILESS_MAIN(FileSystemTest)

#include "FileSystem_test.moc"
