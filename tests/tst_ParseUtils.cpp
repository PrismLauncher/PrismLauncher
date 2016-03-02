#include <QTest>
#include "TestUtil.h"

#include "minecraft/ParseUtils.h"

class ParseUtilsTest : public QObject
{
	Q_OBJECT
private
slots:
	void test_Through_data()
	{
		QTest::addColumn<QString>("timestamp");
		const char * timestamps[] =
		{
			"2016-02-29T13:49:54+01:00",
			"2016-02-26T15:21:11+00:01",
			"2016-02-24T15:52:36+01:13",
			"2016-02-18T17:41:00+00:00",
			"2016-02-17T15:23:19+00:00",
			"2016-02-16T15:22:39+09:22",
			"2016-02-10T15:06:41+00:00",
			"2016-02-04T15:28:02-05:33"
		};
		for(int i = 0; i < (sizeof(timestamps) / sizeof(const char *)); i++)
		{
			QTest::newRow(timestamps[i]) << QString(timestamps[i]);
		}
	}
	void test_Through()
	{
		QFETCH(QString, timestamp);

		auto time_parsed = timeFromS3Time(timestamp);
		auto time_serialized = timeToS3Time(time_parsed);
		
		QCOMPARE(time_serialized, timestamp);
	}

};

QTEST_GUILESS_MAIN(ParseUtilsTest)

#include "tst_ParseUtils.moc"

