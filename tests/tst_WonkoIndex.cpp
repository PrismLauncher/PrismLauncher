#include <QTest>
#include "TestUtil.h"

#include "wonko/WonkoIndex.h"
#include "wonko/WonkoVersionList.h"
#include "Env.h"

class WonkoIndexTest : public QObject
{
	Q_OBJECT
private
slots:
	void test_isProvidedByEnv()
	{
		QVERIFY(ENV.wonkoIndex() != nullptr);
		QCOMPARE(ENV.wonkoIndex(), ENV.wonkoIndex());
	}

	void test_providesTasks()
	{
		QVERIFY(ENV.wonkoIndex()->localUpdateTask() != nullptr);
		QVERIFY(ENV.wonkoIndex()->remoteUpdateTask() != nullptr);
	}

	void test_hasUid_and_getList()
	{
		WonkoIndex windex({std::make_shared<WonkoVersionList>("list1"), std::make_shared<WonkoVersionList>("list2"), std::make_shared<WonkoVersionList>("list3")});
		QVERIFY(windex.hasUid("list1"));
		QVERIFY(!windex.hasUid("asdf"));
		QVERIFY(windex.getList("list2") != nullptr);
		QCOMPARE(windex.getList("list2")->uid(), QString("list2"));
		QVERIFY(windex.getList("adsf") == nullptr);
	}

	void test_merge()
	{
		WonkoIndex windex({std::make_shared<WonkoVersionList>("list1"), std::make_shared<WonkoVersionList>("list2"), std::make_shared<WonkoVersionList>("list3")});
		QCOMPARE(windex.lists().size(), 3);
		windex.merge(std::shared_ptr<WonkoIndex>(new WonkoIndex({std::make_shared<WonkoVersionList>("list1"), std::make_shared<WonkoVersionList>("list2"), std::make_shared<WonkoVersionList>("list3")})));
		QCOMPARE(windex.lists().size(), 3);
		windex.merge(std::shared_ptr<WonkoIndex>(new WonkoIndex({std::make_shared<WonkoVersionList>("list4"), std::make_shared<WonkoVersionList>("list2"), std::make_shared<WonkoVersionList>("list5")})));
		QCOMPARE(windex.lists().size(), 5);
		windex.merge(std::shared_ptr<WonkoIndex>(new WonkoIndex({std::make_shared<WonkoVersionList>("list6")})));
		QCOMPARE(windex.lists().size(), 6);
	}
};

QTEST_GUILESS_MAIN(WonkoIndexTest)

#include "tst_WonkoIndex.moc"
