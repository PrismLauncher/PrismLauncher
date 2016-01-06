#include <QTest>
#include <QTemporaryDir>
#include "TestUtil.h"

#include "FileSystem.h"

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
		QString folder = QFINDTESTDATA("tests/data/test_folder");
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
};

QTEST_GUILESS_MAIN(FileSystemTest)

#include "tst_FileSystem.moc"
