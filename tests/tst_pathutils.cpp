#include <QTest>
#include "TestUtil.h"

#include "depends/util/include/pathutils.h"

class PathUtilsTest : public QObject
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

	void test_PathCombine1_data()
	{
		QTest::addColumn<QString>("result");
		QTest::addColumn<QString>("path1");
		QTest::addColumn<QString>("path2");

#if defined(Q_OS_UNIX)
		QTest::newRow("unix 1") << "/abc/def/ghi/jkl" << "/abc/def" << "ghi/jkl";
		QTest::newRow("unix 2") << "/abc/def/ghi/jkl" << "/abc/def/" << "ghi/jkl";
#elif defined(Q_OS_WIN)
		QTest::newRow("win, from C:") << "C:\\abc" << "C:" << "abc\\def";
		QTest::newRow("win 1") << "C:\\abc\\def\\ghi\\jkl" << "C:\\abc\\def" << "ghi\\jkl";
		QTest::newRow("win 2") << "C:\\abc\\def\\ghi\\jkl" << "C:\\abc\\def\\" << "ghi\\jkl";
#endif
	}
	void test_PathCombine1()
	{
		QFETCH(QString, result);
		QFETCH(QString, path1);
		QFETCH(QString, path2);

		QCOMPARE(PathCombine(path1, path2), result);
	}

	void test_PathCombine2_data()
	{
		QTest::addColumn<QString>("result");
		QTest::addColumn<QString>("path1");
		QTest::addColumn<QString>("path2");
		QTest::addColumn<QString>("path3");

#if defined(Q_OS_UNIX)
		QTest::newRow("unix 1") << "/abc/def/ghi/jkl" << "/abc" << "def" << "ghi/jkl";
		QTest::newRow("unix 2") << "/abc/def/ghi/jkl" << "/abc/" << "def" << "ghi/jkl";
		QTest::newRow("unix 3") << "/abc/def/ghi/jkl" << "/abc" << "def/" << "ghi/jkl";
		QTest::newRow("unix 4") << "/abc/def/ghi/jkl" << "/abc/" << "def/" << "ghi/jkl";
#elif defined(Q_OS_WIN)
		QTest::newRow("win 1") << "C:\\abc\\def\\ghi\\jkl" << "C:\\abc" << "def" << "ghi\\jkl";
		QTest::newRow("win 2") << "C:\\abc\\def\\ghi\\jkl" << "C:\\abc\\" << "def" << "ghi\\jkl";
		QTest::newRow("win 3") << "C:\\abc\\def\\ghi\\jkl" << "C:\\abc" << "def\\" << "ghi\\jkl";
		QTest::newRow("win 4") << "C:\\abc\\def\\ghi\\jkl" << "C:\\abc\\" << "def" << "ghi\\jkl";
#endif
	}
	void test_PathCombine2()
	{
		QFETCH(QString, result);
		QFETCH(QString, path1);
		QFETCH(QString, path2);
		QFETCH(QString, path3);

		QCOMPARE(PathCombine(path1, path2, path3), result);
	}
};

QTEST_GUILESS_MAIN_MULTIMC(PathUtilsTest)

#include "tst_pathutils.moc"
