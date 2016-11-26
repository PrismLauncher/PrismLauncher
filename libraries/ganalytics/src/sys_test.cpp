#include <QTest>
#include "TestUtil.h"

#include <sys.h>

class SysTest : public QObject
{
	Q_OBJECT
private
slots:

	void test_kernelNotNull()
	{
		auto kinfo = Sys::getKernelInfo();
		QVERIFY(!kinfo.kernelName.isEmpty());
		QVERIFY(kinfo.kernelVersion != "0.0");
	}
};

QTEST_GUILESS_MAIN(SysTest)

#include "sys_test.moc"
