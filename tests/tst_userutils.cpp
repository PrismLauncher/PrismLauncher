#include <QTest>
#include <QStandardPaths>
#include "TestUtil.h"

#include "userutils.h"

class UserUtilsTest : public QObject
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

	void test_getDesktop()
	{
		QCOMPARE(Util::getDesktopDir(), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
	}

// this is only valid on linux
// FIXME: implement on windows, OSX, then test.
#if defined(Q_OS_LINUX)
	void test_createShortcut_data()
	{
		QTest::addColumn<QString>("location");
		QTest::addColumn<QString>("dest");
		QTest::addColumn<QStringList>("args");
		QTest::addColumn<QString>("name");
		QTest::addColumn<QString>("iconLocation");
		QTest::addColumn<QByteArray>("result");

		QTest::newRow("unix") << QDir::currentPath()
							  << "asdfDest"
							  << (QStringList() << "arg1" << "arg2")
							  << "asdf"
							  << QString()
						 #if defined(Q_OS_LINUX)
							  << MULTIMC_GET_TEST_FILE("data/tst_userutils-test_createShortcut-unix")
						 #elif defined(Q_OS_WIN)
							  << QByteArray()
						 #endif
								 ;
	}

	void test_createShortcut()
	{
		QFETCH(QString, location);
		QFETCH(QString, dest);
		QFETCH(QStringList, args);
		QFETCH(QString, name);
		QFETCH(QString, iconLocation);
		QFETCH(QByteArray, result);

		QVERIFY(Util::createShortCut(location, dest, args, name, iconLocation));
		QCOMPARE(QString::fromLocal8Bit(TestsInternal::readFile(location + QDir::separator() + name + ".desktop")), QString::fromLocal8Bit(result));

		//QDir().remove(location);
	}
#endif
};


QTEST_GUILESS_MAIN(UserUtilsTest)

#include "tst_userutils.moc"
