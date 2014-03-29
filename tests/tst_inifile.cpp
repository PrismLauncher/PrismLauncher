#include <QTest>
#include "TestUtil.h"

#include "depends/settings/inifile.h"

class IniFileTest : public QObject
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
		QTest::addColumn<QString>("through");

		QTest::newRow("unix path") << "/abc/def/ghi/jkl";
		QTest::newRow("windows path") << "C:\\Program files\\terrible\\name\\of something\\";
		QTest::newRow("Plain text") << "Lorem ipsum dolor sit amet.";
		QTest::newRow("Escape sequences") << "Lorem\n\t\n\\n\\tAAZ\nipsum dolor\n\nsit amet.";
		QTest::newRow("Escape sequences 2") << "\"\n\n\"";
	}
	void test_PathCombine1()
	{
		QFETCH(QString, through);

		QString there = INIFile::escape(through);
		QString back = INIFile::unescape(there);
		
		QCOMPARE(back, through);
	}
};

QTEST_GUILESS_MAIN_MULTIMC(IniFileTest)

#include "tst_inifile.moc"
