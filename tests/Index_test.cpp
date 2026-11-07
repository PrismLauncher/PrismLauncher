#include <QTest>

#include <meta/Index.h>
#include <meta/VersionList.h>

class IndexTest : public QObject {
    Q_OBJECT
   private slots:
    void test_hasUid_and_getList()
    {
        Meta::Index windex({ std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"),
                             std::make_shared<Meta::VersionList>("list3") });
        QVERIFY(windex.hasUid("list1"));
        QVERIFY(!windex.hasUid("asdf"));
        QVERIFY(windex.get("list2") != nullptr);
        QCOMPARE(windex.get("list2")->uid(), QString("list2"));
        QVERIFY(windex.get("adsf") != nullptr);
    }

    void test_merge()
    {
        Meta::Index windex({ std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"),
                             std::make_shared<Meta::VersionList>("list3") });
        QCOMPARE(windex.lists().size(), 3);
        windex.merge(std::shared_ptr<Meta::Index>(
            new Meta::Index({ std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"),
                              std::make_shared<Meta::VersionList>("list3") })));
        QCOMPARE(windex.lists().size(), 3);
        windex.merge(std::shared_ptr<Meta::Index>(
            new Meta::Index({ std::make_shared<Meta::VersionList>("list4"), std::make_shared<Meta::VersionList>("list2"),
                              std::make_shared<Meta::VersionList>("list5") })));
        QCOMPARE(windex.lists().size(), 5);
        windex.merge(std::shared_ptr<Meta::Index>(new Meta::Index({ std::make_shared<Meta::VersionList>("list6") })));
        QCOMPARE(windex.lists().size(), 6);
    }
    void test_loadVersion_offline()
    {
        Meta::Index windex;
        QDir().mkpath("meta/net.minecraft");
        QFile file("meta/net.minecraft/testver.json");
        QVERIFY(file.open(QFile::WriteOnly));
        file.write("{\"formatVersion\": 1, \"uid\": \"net.minecraft\", \"version\": \"testver\", \"releaseTime\": \"2023-12-06T14:38:01+00:00\"}");
        file.close();

        auto task = windex.loadVersion("net.minecraft", "testver", Net::Mode::Offline);
        QVERIFY(task != nullptr);
        task->start();
        QVERIFY2(QTest::qWaitFor([&task]() { return task->isFinished(); }, 1000), "Task didn't finish");
        QVERIFY(task->wasSuccessful());

        auto version = windex.get("net.minecraft", "testver");
        QVERIFY(version != nullptr);
        QVERIFY(version->data() != nullptr);
        QCOMPARE(version->status(), Meta::BaseEntity::LoadStatus::Local);

        QFile::remove("meta/net.minecraft/testver.json");
        QDir().rmdir("meta/net.minecraft");
        QDir().rmdir("meta");
    }
    void test_loadVersion_offline_missing()
    {
        Meta::Index windex;
        auto task = windex.loadVersion("net.minecraft", "missingver", Net::Mode::Offline);
        QVERIFY(task != nullptr);
        task->start();
        QVERIFY2(QTest::qWaitFor([&task]() { return task->isFinished(); }, 1000), "Task didn't finish");
        QVERIFY(!task->wasSuccessful());
    }
};

QTEST_GUILESS_MAIN(IndexTest)

#include "Index_test.moc"
