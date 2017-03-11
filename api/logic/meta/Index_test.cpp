#include <QTest>
#include "TestUtil.h"

#include "meta/Index.h"
#include "meta/VersionList.h"
#include "Env.h"

class IndexTest : public QObject
{
	Q_OBJECT
private
slots:
	void test_isProvidedByEnv()
	{
		QVERIFY(ENV.metadataIndex());
		QCOMPARE(ENV.metadataIndex(), ENV.metadataIndex());
	}

	void test_providesTasks()
	{
		QVERIFY(ENV.metadataIndex()->localUpdateTask() != nullptr);
		QVERIFY(ENV.metadataIndex()->remoteUpdateTask() != nullptr);
	}

	void test_hasUid_and_getList()
	{
		Meta::Index windex({std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"), std::make_shared<Meta::VersionList>("list3")});
		QVERIFY(windex.hasUid("list1"));
		QVERIFY(!windex.hasUid("asdf"));
		QVERIFY(windex.getList("list2") != nullptr);
		QCOMPARE(windex.getList("list2")->uid(), QString("list2"));
		QVERIFY(windex.getList("adsf") == nullptr);
	}

	void test_merge()
	{
		Meta::Index windex({std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"), std::make_shared<Meta::VersionList>("list3")});
		QCOMPARE(windex.lists().size(), 3);
		windex.merge(std::shared_ptr<Meta::Index>(new Meta::Index({std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"), std::make_shared<Meta::VersionList>("list3")})));
		QCOMPARE(windex.lists().size(), 3);
		windex.merge(std::shared_ptr<Meta::Index>(new Meta::Index({std::make_shared<Meta::VersionList>("list4"), std::make_shared<Meta::VersionList>("list2"), std::make_shared<Meta::VersionList>("list5")})));
		QCOMPARE(windex.lists().size(), 5);
		windex.merge(std::shared_ptr<Meta::Index>(new Meta::Index({std::make_shared<Meta::VersionList>("list6")})));
		QCOMPARE(windex.lists().size(), 6);
	}
};

QTEST_GUILESS_MAIN(IndexTest)

#include "Index_test.moc"
