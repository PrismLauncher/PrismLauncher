#include <QTest>
#include <QSignalSpy>

#include "depends/settings/settingsobject.h"
#include "depends/settings/setting.h"

#include "Config.h"
#include "TestUtil.h"
#include "logic/updater/UpdateChecker.h"

Q_DECLARE_METATYPE(UpdateChecker::ChannelListEntry)

bool operator==(const UpdateChecker::ChannelListEntry &e1, const UpdateChecker::ChannelListEntry &e2)
{
	return e1.id == e2.id &&
			e1.name == e2.name &&
			e1.description == e2.description &&
			e1.url == e2.url;
}

QDebug operator<<(QDebug dbg, const UpdateChecker::ChannelListEntry &c)
{
	dbg.nospace() << "ChannelListEntry(id=" << c.id << " name=" << c.name << " description=" << c.description << " url=" << c.url << ")";
	return dbg.maybeSpace();
}

class ResetSetting
{
public:
	ResetSetting(std::shared_ptr<Setting> setting) : setting(setting), oldValue(setting->get()) {}
	~ResetSetting()
	{
		setting->set(oldValue);
	}

	std::shared_ptr<Setting> setting;
	QVariant oldValue;
};

class UpdateCheckerTest : public QObject
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

	static QString findTestDataUrl(const char *file)
	{
		return QUrl::fromLocalFile(QFINDTESTDATA(file)).toString();
	}
	void tst_ChannelListParsing_data()
	{
		QTest::addColumn<QString>("channel");
		QTest::addColumn<QString>("channelUrl");
		QTest::addColumn<bool>("hasChannels");
		QTest::addColumn<bool>("valid");
		QTest::addColumn<QList<UpdateChecker::ChannelListEntry> >("result");

		QTest::newRow("garbage")
				<< QString()
				<< findTestDataUrl("tests/data/garbageChannels.json")
				<< false
				<< false
				<< QList<UpdateChecker::ChannelListEntry>();
		QTest::newRow("errors")
				<< QString()
				<< findTestDataUrl("tests/data/errorChannels.json")
				<< false
				<< true
				<< QList<UpdateChecker::ChannelListEntry>();
		QTest::newRow("no channels")
				<< QString()
				<< findTestDataUrl("tests/data/noChannels.json")
				<< false
				<< true
				<< QList<UpdateChecker::ChannelListEntry>();
		QTest::newRow("one channel")
				<< QString("develop")
				<< findTestDataUrl("tests/data/oneChannel.json")
				<< true
				<< true
				<< (QList<UpdateChecker::ChannelListEntry>() << UpdateChecker::ChannelListEntry{"develop", "Develop", "The channel called \"develop\"", "http://example.org/stuff"});
		QTest::newRow("several channels")
				<< QString("develop")
				<< findTestDataUrl("tests/data/channels.json")
				<< true
				<< true
				<< (QList<UpdateChecker::ChannelListEntry>()
					<< UpdateChecker::ChannelListEntry{"develop", "Develop", "The channel called \"develop\"", MultiMC_TEST_DATA_PATH}
					<< UpdateChecker::ChannelListEntry{"stable", "Stable", "It's stable at least", MultiMC_TEST_DATA_PATH}
					<< UpdateChecker::ChannelListEntry{"42", "The Channel", "This is the channel that is going to answer all of your questions", "https://dent.me/tea"});
	}
	void tst_ChannelListParsing()
	{
		ResetSetting resetUpdateChannel(MMC->settings()->getSetting("UpdateChannel"));

		QFETCH(QString, channel);
		QFETCH(QString, channelUrl);
		QFETCH(bool, hasChannels);
		QFETCH(bool, valid);
		QFETCH(QList<UpdateChecker::ChannelListEntry>, result);

		MMC->settings()->set("UpdateChannel", channel);

		UpdateChecker checker;

		QSignalSpy channelListLoadedSpy(&checker, SIGNAL(channelListLoaded()));
		QVERIFY(channelListLoadedSpy.isValid());

		checker.setChannelListUrl(channelUrl);

		checker.updateChanList();

		if (valid)
		{
			QVERIFY(channelListLoadedSpy.wait());
			QCOMPARE(channelListLoadedSpy.size(), 1);
		}
		else
		{
			channelListLoadedSpy.wait();
			QCOMPARE(channelListLoadedSpy.size(), 0);
		}

		QCOMPARE(checker.hasChannels(), hasChannels);
		QCOMPARE(checker.getChannelList(), result);
	}

	void tst_UpdateChecking_data()
	{
		QTest::addColumn<QString>("channel");
		QTest::addColumn<QString>("channelUrl");
		QTest::addColumn<int>("currentBuild");
		QTest::addColumn<QList<QVariant> >("result");

		QTest::newRow("valid channel")
				<< "develop" << findTestDataUrl("tests/data/channels.json")
				<< 2
				<< (QList<QVariant>() << QString() << "1.0.3" << 3);
	}
	void tst_UpdateChecking()
	{
		ResetSetting resetUpdateChannel(MMC->settings()->getSetting("UpdateChannel"));

		QFETCH(QString, channel);
		QFETCH(QString, channelUrl);
		QFETCH(int, currentBuild);
		QFETCH(QList<QVariant>, result);

		MMC->settings()->set("UpdateChannel", channel);
		BuildConfig.VERSION_BUILD = currentBuild;

		UpdateChecker checker;
		checker.setChannelListUrl(channelUrl);

		QSignalSpy updateAvailableSpy(&checker, SIGNAL(updateAvailable(QString,QString,int)));
		QVERIFY(updateAvailableSpy.isValid());
		QSignalSpy channelListLoadedSpy(&checker, SIGNAL(channelListLoaded()));
		QVERIFY(channelListLoadedSpy.isValid());

		checker.updateChanList();
		QVERIFY(channelListLoadedSpy.wait());

		checker.m_channels[0].url = QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/")).toString();

		checker.checkForUpdate(false);

		QVERIFY(updateAvailableSpy.wait());
		QList<QVariant> res = result;
		res[0] = checker.m_channels[0].url;
		QCOMPARE(updateAvailableSpy.first(), res);
	}
};

QTEST_GUILESS_MAIN_MULTIMC(UpdateCheckerTest)

#include "tst_UpdateChecker.moc"
