#include <QTest>
#include "TestUtil.h"

#include "pathutils.h"

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

		QCOMPARE(PathCombine(path1, path2), result);
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

		QCOMPARE(PathCombine(path1, path2, path3), result);
	}
};

QTEST_GUILESS_MAIN(PathUtilsTest)

#include "tst_pathutils.moc"
