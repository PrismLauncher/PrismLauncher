#include <QTest>
#include <QSignalSpy>

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
		QTest::addColumn<QList<UpdateChecker::ChannelListEntry> >("result");

		QTest::newRow("no channels")
				<< QString()
				<< findTestDataUrl("tests/data/noChannels.json")
				<< false
				<< QList<UpdateChecker::ChannelListEntry>();
		QTest::newRow("one channel")
				<< QString("develop")
				<< findTestDataUrl("tests/data/oneChannel.json")
				<< true
				<< (QList<UpdateChecker::ChannelListEntry>() << UpdateChecker::ChannelListEntry{"develop", "Develop", "The channel called \"develop\"", "http://example.org/stuff"});
		QTest::newRow("several channels")
				<< QString("develop")
				<< findTestDataUrl("tests/data/channels.json")
				<< true
				<< (QList<UpdateChecker::ChannelListEntry>()
					<< UpdateChecker::ChannelListEntry{"develop", "Develop", "The channel called \"develop\"", "http://example.org/stuff"}
					<< UpdateChecker::ChannelListEntry{"stable", "Stable", "It's stable at least", "ftp://username@host/path/to/stuff"}
					<< UpdateChecker::ChannelListEntry{"42", "The Channel", "This is the channel that is going to answer all of your questions", "https://dent.me/tea"});
	}
	void tst_ChannelListParsing()
	{
		QFETCH(QString, channel);
		QFETCH(QString, channelUrl);
		QFETCH(bool, hasChannels);
		QFETCH(QList<UpdateChecker::ChannelListEntry>, result);

		UpdateChecker checker;

		QSignalSpy spy(&checker, SIGNAL(channelListLoaded()));
		QVERIFY(spy.isValid());

		checker.setCurrentChannel(channel);
		checker.setChannelListUrl(channelUrl);

		checker.updateChanList();

		QVERIFY(spy.wait());

		QCOMPARE(spy.size(), 1);

		QCOMPARE(checker.hasChannels(), hasChannels);
		QCOMPARE(checker.getChannelList(), result);
	}
};

QTEST_GUILESS_MAIN_MULTIMC(UpdateCheckerTest)

#include "tst_UpdateChecker.moc"
