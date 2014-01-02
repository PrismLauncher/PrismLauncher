#include <QTest>
#include <QSignalSpy>

#include "TestUtil.h"

#include "logic/updater/DownloadUpdateTask.h"
#include "logic/updater/UpdateChecker.h"
#include "depends/util/include/pathutils.h"

DownloadUpdateTask::FileSourceList encodeBaseFile(const char *suffix)
{
	auto base = qApp->applicationDirPath();
	QUrl localFile = QUrl::fromLocalFile(base + suffix);
	QString localUrlString = localFile.toString(QUrl::FullyEncoded);
	auto item = DownloadUpdateTask::FileSource("http", localUrlString);
	return DownloadUpdateTask::FileSourceList({item});
}

Q_DECLARE_METATYPE(DownloadUpdateTask::VersionFileList)
Q_DECLARE_METATYPE(DownloadUpdateTask::UpdateOperation)

bool operator==(const DownloadUpdateTask::FileSource &f1,
				const DownloadUpdateTask::FileSource &f2)
{
	return f1.type == f2.type && f1.url == f2.url && f1.compressionType == f2.compressionType;
}
bool operator==(const DownloadUpdateTask::VersionFileEntry &v1,
				const DownloadUpdateTask::VersionFileEntry &v2)
{
	return v1.path == v2.path && v1.mode == v2.mode && v1.sources == v2.sources &&
		   v1.md5 == v2.md5;
}
bool operator==(const DownloadUpdateTask::UpdateOperation &u1,
				const DownloadUpdateTask::UpdateOperation &u2)
{
	return u1.type == u2.type && u1.file == u2.file && u1.dest == u2.dest && u1.mode == u2.mode;
}

QDebug operator<<(QDebug dbg, const DownloadUpdateTask::FileSource &f)
{
	dbg.nospace() << "FileSource(type=" << f.type << " url=" << f.url
				  << " comp=" << f.compressionType << ")";
	return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const DownloadUpdateTask::VersionFileEntry &v)
{
	dbg.nospace() << "VersionFileEntry(path=" << v.path << " mode=" << v.mode
				  << " md5=" << v.md5 << " sources=" << v.sources << ")";
	return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const DownloadUpdateTask::UpdateOperation::Type &t)
{
	switch (t)
	{
	case DownloadUpdateTask::UpdateOperation::OP_COPY:
		dbg << "OP_COPY";
		break;
	case DownloadUpdateTask::UpdateOperation::OP_DELETE:
		dbg << "OP_DELETE";
		break;
	case DownloadUpdateTask::UpdateOperation::OP_MOVE:
		dbg << "OP_MOVE";
		break;
	case DownloadUpdateTask::UpdateOperation::OP_CHMOD:
		dbg << "OP_CHMOD";
		break;
	}
	return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const DownloadUpdateTask::UpdateOperation &u)
{
	dbg.nospace() << "UpdateOperation(type=" << u.type << " file=" << u.file
				  << " dest=" << u.dest << " mode=" << u.mode << ")";
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
		DownloadUpdateTask task(
			QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/")).toString(), 0);

		DownloadUpdateTask::UpdateOperationList ops;

		ops << DownloadUpdateTask::UpdateOperation::CopyOp("sourceOne", "destOne", 0777)
			<< DownloadUpdateTask::UpdateOperation::CopyOp("MultiMC.exe", "M/u/l/t/i/M/C/e/x/e")
			<< DownloadUpdateTask::UpdateOperation::DeleteOp("toDelete.abc");
		auto testFile = "tests/data/tst_DownloadUpdateTask-test_writeInstallScript.xml";
		const QString script = QDir::temp().absoluteFilePath("MultiMCUpdateScript.xml");
		QVERIFY(task.writeInstallScript(ops, script));
		QCOMPARE(TestsInternal::readFileUtf8(script).replace(QRegExp("[\r\n]+"), "\n"),
				 MULTIMC_GET_TEST_FILE_UTF8(testFile).replace(QRegExp("[\r\n]+"), "\n"));
	}

	void test_parseVersionInfo_data()
	{
		QTest::addColumn<QByteArray>("data");
		QTest::addColumn<DownloadUpdateTask::VersionFileList>("list");
		QTest::addColumn<QString>("error");
		QTest::addColumn<bool>("ret");

		QTest::newRow("one")
			<< MULTIMC_GET_TEST_FILE("tests/data/1.json")
			<< (DownloadUpdateTask::VersionFileList()
				<< DownloadUpdateTask::VersionFileEntry{"fileOne",
														493,
														encodeBaseFile("/tests/data/fileOneA"),
														"9eb84090956c484e32cb6c08455a667b"}
				<< DownloadUpdateTask::VersionFileEntry{"fileTwo",
														644,
														encodeBaseFile("/tests/data/fileTwo"),
														"38f94f54fa3eb72b0ea836538c10b043"}
				<< DownloadUpdateTask::VersionFileEntry{"fileThree",
														750,
														encodeBaseFile("/tests/data/fileThree"),
														"f12df554b21e320be6471d7154130e70"})
			<< QString() << true;
		QTest::newRow("two")
			<< MULTIMC_GET_TEST_FILE("tests/data/2.json")
			<< (DownloadUpdateTask::VersionFileList()
				<< DownloadUpdateTask::VersionFileEntry{"fileOne",
														493,
														encodeBaseFile("/tests/data/fileOneB"),
														"42915a71277c9016668cce7b82c6b577"}
				<< DownloadUpdateTask::VersionFileEntry{"fileTwo",
														644,
														encodeBaseFile("/tests/data/fileTwo"),
														"38f94f54fa3eb72b0ea836538c10b043"})
			<< QString() << true;
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

	void test_processFileLists_data()
	{
		QTest::addColumn<DownloadUpdateTask *>("downloader");
		QTest::addColumn<DownloadUpdateTask::VersionFileList>("currentVersion");
		QTest::addColumn<DownloadUpdateTask::VersionFileList>("newVersion");
		QTest::addColumn<DownloadUpdateTask::UpdateOperationList>("expectedOperations");

		DownloadUpdateTask *downloader = new DownloadUpdateTask(QString(), -1);

		// update fileOne, keep fileTwo, remove fileThree
		QTest::newRow("test 1")
			<< downloader << (DownloadUpdateTask::VersionFileList()
							  << DownloadUpdateTask::VersionFileEntry{
									 "tests/data/fileOne", 493,
									 DownloadUpdateTask::FileSourceList()
										 << DownloadUpdateTask::FileSource(
												"http", "http://host/path/fileOne-1"),
									 "9eb84090956c484e32cb6c08455a667b"}
							  << DownloadUpdateTask::VersionFileEntry{
									 "tests/data/fileTwo", 644,
									 DownloadUpdateTask::FileSourceList()
										 << DownloadUpdateTask::FileSource(
												"http", "http://host/path/fileTwo-1"),
									 "38f94f54fa3eb72b0ea836538c10b043"}
							  << DownloadUpdateTask::VersionFileEntry{
									 "tests/data/fileThree", 420,
									 DownloadUpdateTask::FileSourceList()
										 << DownloadUpdateTask::FileSource(
												"http", "http://host/path/fileThree-1"),
									 "f12df554b21e320be6471d7154130e70"})
			<< (DownloadUpdateTask::VersionFileList()
				<< DownloadUpdateTask::VersionFileEntry{
					   "tests/data/fileOne", 493,
					   DownloadUpdateTask::FileSourceList()
						   << DownloadUpdateTask::FileSource("http",
															 "http://host/path/fileOne-2"),
					   "42915a71277c9016668cce7b82c6b577"}
				<< DownloadUpdateTask::VersionFileEntry{
					   "tests/data/fileTwo", 644,
					   DownloadUpdateTask::FileSourceList()
						   << DownloadUpdateTask::FileSource("http",
															 "http://host/path/fileTwo-2"),
					   "38f94f54fa3eb72b0ea836538c10b043"})
			<< (DownloadUpdateTask::UpdateOperationList()
				<< DownloadUpdateTask::UpdateOperation::DeleteOp("tests/data/fileThree")
				<< DownloadUpdateTask::UpdateOperation::CopyOp(
					   PathCombine(downloader->updateFilesDir(),
								   QString("tests/data/fileOne").replace("/", "_")),
					   "tests/data/fileOne", 493));
	}
	void test_processFileLists()
	{
		QFETCH(DownloadUpdateTask *, downloader);
		QFETCH(DownloadUpdateTask::VersionFileList, currentVersion);
		QFETCH(DownloadUpdateTask::VersionFileList, newVersion);
		QFETCH(DownloadUpdateTask::UpdateOperationList, expectedOperations);

		DownloadUpdateTask::UpdateOperationList operations;

		downloader->processFileLists(new NetJob("Dummy"), currentVersion, newVersion,
									 operations);
		qDebug() << (operations == expectedOperations);
		qDebug() << operations;
		qDebug() << expectedOperations;
		QCOMPARE(operations, expectedOperations);
	}

	void test_masterTest()
	{
		QLOG_INFO() << "#####################";
		MMC->m_version.build = 1;
		MMC->m_version.channel = "develop";
		auto channels =
			QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/channels.json"));
		auto root = QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/"));
		QLOG_DEBUG() << "channels: " << channels;
		QLOG_DEBUG() << "root: " << root;
		MMC->updateChecker()->setChannelListUrl(channels.toString());
		MMC->updateChecker()->setCurrentChannel("develop");

		DownloadUpdateTask task(root.toString(), 2);

		QSignalSpy succeededSpy(&task, SIGNAL(succeeded()));

		task.start();

		QVERIFY(succeededSpy.wait());
	}

	void test_OSXPathFixup()
	{
		QString path, pathOrig;
		bool result;
		// Proper OSX path
		pathOrig = path = "MultiMC.app/Foo/Bar/Baz";
		qDebug() << "Proper OSX path: " << path;
		result = DownloadUpdateTask::fixPathForOSX(path);
		QCOMPARE(path, QString("../../Foo/Bar/Baz"));
		QCOMPARE(result, true);

		// Bad OSX path
		pathOrig = path = "translations/klingon.lol";
		qDebug() << "Bad OSX path: " << path;
		result = DownloadUpdateTask::fixPathForOSX(path);
		QCOMPARE(path, pathOrig);
		QCOMPARE(result, false);
	}
};

QTEST_GUILESS_MAIN_MULTIMC(DownloadUpdateTaskTest)

#include "tst_DownloadUpdateTask.moc"
