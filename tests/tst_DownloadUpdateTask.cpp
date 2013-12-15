#include <QTest>
#include <QSignalSpy>

#include "TestUtil.h"

#include "logic/updater/DownloadUpdateTask.h"
#include "logic/updater/UpdateChecker.h"

Q_DECLARE_METATYPE(DownloadUpdateTask::VersionFileList)

bool operator==(const DownloadUpdateTask::FileSource &f1, const DownloadUpdateTask::FileSource &f2)
{
	return f1.type == f2.type &&
			f1.url == f2.url &&
			f1.compressionType == f2.compressionType;
}
bool operator==(const DownloadUpdateTask::VersionFileEntry &v1, const DownloadUpdateTask::VersionFileEntry &v2)
{
	return v1.path == v2.path &&
			v1.mode == v2.mode &&
			v1.sources == v2.sources &&
			v1.md5 == v2.md5;
}

QDebug operator<<(QDebug dbg, const DownloadUpdateTask::FileSource &f)
{
	dbg.nospace() << "FileSource(type=" << f.type << " url=" << f.url << " comp=" << f.compressionType << ")";
	return dbg.maybeSpace();
}
QDebug operator<<(QDebug dbg, const DownloadUpdateTask::VersionFileEntry &v)
{
	dbg.nospace() << "VersionFileEntry(path=" << v.path << " mode=" << v.mode << " md5=" << v.md5 << " sources=" << v.sources << ")";
	return dbg.maybeSpace();
}

class DownloadUpdateTaskTest : public QObject
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

	void test_writeInstallScript()
	{
		DownloadUpdateTask task(QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/")).toString(), 0);

		DownloadUpdateTask::UpdateOperationList ops;

		ops << DownloadUpdateTask::UpdateOperation::CopyOp("sourceOne", "destOne", 0777)
			<< DownloadUpdateTask::UpdateOperation::CopyOp("MultiMC.exe", "M/u/l/t/i/M/C/e/x/e")
			<< DownloadUpdateTask::UpdateOperation::DeleteOp("toDelete.abc");

		const QString script = QDir::temp().absoluteFilePath("MultiMCUpdateScript.xml");
		QVERIFY(task.writeInstallScript(ops, script));
		QCOMPARE(TestsInternal::readFileUtf8(script), MULTIMC_GET_TEST_FILE_UTF8("tests/data/tst_DownloadUpdateTask-test_writeInstallScript.xml"));
	}

	void test_parseVersionInfo_data()
	{
		QTest::addColumn<QByteArray>("data");
		QTest::addColumn<DownloadUpdateTask::VersionFileList>("list");
		QTest::addColumn<QString>("error");
		QTest::addColumn<bool>("ret");

		QTest::newRow("one") << MULTIMC_GET_TEST_FILE("tests/data/1.json")
							 << (DownloadUpdateTask::VersionFileList()
								 << DownloadUpdateTask::VersionFileEntry{"fileOne", 493,
																		 (DownloadUpdateTask::FileSourceList() << DownloadUpdateTask::FileSource("http", "file://" + qApp->applicationDirPath() + "/tests/data/fileOneA")),
																		 "9eb84090956c484e32cb6c08455a667b"}
								 << DownloadUpdateTask::VersionFileEntry{"fileTwo", 644,
																		 (DownloadUpdateTask::FileSourceList() << DownloadUpdateTask::FileSource("http", "file://" + qApp->applicationDirPath() + "/tests/data/fileTwo")),
																		 "38f94f54fa3eb72b0ea836538c10b043"}
								 << DownloadUpdateTask::VersionFileEntry{"fileThree", 750,
																		 (DownloadUpdateTask::FileSourceList() << DownloadUpdateTask::FileSource("http", "file://" + qApp->applicationDirPath() + "/tests/data/fileThree")),
																		 "f12df554b21e320be6471d7154130e70"})
							 << QString()
							 << true;
		QTest::newRow("two") << MULTIMC_GET_TEST_FILE("tests/data/2.json")
							 << (DownloadUpdateTask::VersionFileList()
								 << DownloadUpdateTask::VersionFileEntry{"fileOne", 493,
																		 (DownloadUpdateTask::FileSourceList() << DownloadUpdateTask::FileSource("http", "file://" + qApp->applicationDirPath() + "/tests/data/fileOneB")),
																		 "42915a71277c9016668cce7b82c6b577"}
								 << DownloadUpdateTask::VersionFileEntry{"fileTwo", 644,
																		 (DownloadUpdateTask::FileSourceList() << DownloadUpdateTask::FileSource("http", "file://" + qApp->applicationDirPath() + "/tests/data/fileTwo")),
																		 "38f94f54fa3eb72b0ea836538c10b043"})
							 << QString()
							 << true;
	}
	void test_parseVersionInfo()
	{
		QFETCH(QByteArray, data);
		QFETCH(DownloadUpdateTask::VersionFileList, list);
		QFETCH(QString, error);
		QFETCH(bool, ret);

		DownloadUpdateTask::VersionFileList outList;
		QString outError;
		bool outRet = DownloadUpdateTask("", 0).parseVersionInfo(data, &outList, &outError);
		QCOMPARE(outRet, ret);
		QCOMPARE(outList, list);
		QCOMPARE(outError, error);
	}

	void test_processFileLists()
	{
		// TODO create unit test for this
	}

	void test_masterTest()
	{
		QLOG_INFO() << "#####################";
		MMC->m_version.build = 1;
		MMC->m_version.channel = "develop";
		MMC->updateChecker()->setChannelListUrl(QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/channels.json")).toString());
		MMC->updateChecker()->setCurrentChannel("develop");

		DownloadUpdateTask task(QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/")).toString(), 2);

		QSignalSpy succeededSpy(&task, SIGNAL(succeeded()));

		task.start();

		QVERIFY(succeededSpy.wait());
	}
};

QTEST_GUILESS_MAIN_MULTIMC(DownloadUpdateTaskTest)

#include "tst_DownloadUpdateTask.moc"
