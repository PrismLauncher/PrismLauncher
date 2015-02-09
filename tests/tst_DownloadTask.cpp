#include <QTest>
#include <QSignalSpy>

#include "TestUtil.h"

#include "updater/GoUpdate.h"
#include "updater/DownloadTask.h"
#include "updater/UpdateChecker.h"
#include "pathutils.h"

using namespace GoUpdate;

FileSourceList encodeBaseFile(const char *suffix)
{
	auto base = qApp->applicationDirPath();
	QUrl localFile = QUrl::fromLocalFile(base + suffix);
	QString localUrlString = localFile.toString(QUrl::FullyEncoded);
	auto item = FileSource("http", localUrlString);
	return FileSourceList({item});
}

Q_DECLARE_METATYPE(VersionFileList)
Q_DECLARE_METATYPE(Operation)

QDebug operator<<(QDebug dbg, const FileSource &f)
{
	dbg.nospace() << "FileSource(type=" << f.type << " url=" << f.url
				  << " comp=" << f.compressionType << ")";
	return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const VersionFileEntry &v)
{
	dbg.nospace() << "VersionFileEntry(path=" << v.path << " mode=" << v.mode
				  << " md5=" << v.md5 << " sources=" << v.sources << ")";
	return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const Operation::Type &t)
{
	switch (t)
	{
	case Operation::OP_COPY:
		dbg << "OP_COPY";
		break;
	case Operation::OP_DELETE:
		dbg << "OP_DELETE";
		break;
	case Operation::OP_MOVE:
		dbg << "OP_MOVE";
		break;
	case Operation::OP_CHMOD:
		dbg << "OP_CHMOD";
		break;
	}
	return dbg.maybeSpace();
}

QDebug operator<<(QDebug dbg, const Operation &u)
{
	dbg.nospace() << "Operation(type=" << u.type << " file=" << u.file
				  << " dest=" << u.dest << " mode=" << u.mode << ")";
	return dbg.maybeSpace();
}

class DownloadTaskTest : public QObject
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
		OperationList ops;

		ops << Operation::CopyOp("sourceOne", "destOne", 0777)
			<< Operation::CopyOp("MultiMC.exe", "M/u/l/t/i/M/C/e/x/e")
			<< Operation::DeleteOp("toDelete.abc");
		auto testFile = "tests/data/tst_DownloadTask-test_writeInstallScript.xml";
		const QString script = QDir::temp().absoluteFilePath("MultiMCUpdateScript.xml");
		QVERIFY(writeInstallScript(ops, script));
		QCOMPARE(TestsInternal::readFileUtf8(script).replace(QRegExp("[\r\n]+"), "\n"),
				 MULTIMC_GET_TEST_FILE_UTF8(testFile).replace(QRegExp("[\r\n]+"), "\n"));
	}

	void test_parseVersionInfo_data()
	{
		QTest::addColumn<QByteArray>("data");
		QTest::addColumn<VersionFileList>("list");
		QTest::addColumn<QString>("error");
		QTest::addColumn<bool>("ret");

		QTest::newRow("one")
			<< MULTIMC_GET_TEST_FILE("tests/data/1.json")
			<< (VersionFileList()
				<< VersionFileEntry{"fileOne",
														493,
														encodeBaseFile("/tests/data/fileOneA"),
														"9eb84090956c484e32cb6c08455a667b"}
				<< VersionFileEntry{"fileTwo",
														644,
														encodeBaseFile("/tests/data/fileTwo"),
														"38f94f54fa3eb72b0ea836538c10b043"}
				<< VersionFileEntry{"fileThree",
														750,
														encodeBaseFile("/tests/data/fileThree"),
														"f12df554b21e320be6471d7154130e70"})
			<< QString() << true;
		QTest::newRow("two")
			<< MULTIMC_GET_TEST_FILE("tests/data/2.json")
			<< (VersionFileList()
				<< VersionFileEntry{"fileOne",
														493,
														encodeBaseFile("/tests/data/fileOneB"),
														"42915a71277c9016668cce7b82c6b577"}
				<< VersionFileEntry{"fileTwo",
														644,
														encodeBaseFile("/tests/data/fileTwo"),
														"38f94f54fa3eb72b0ea836538c10b043"})
			<< QString() << true;
	}
	void test_parseVersionInfo()
	{
		QFETCH(QByteArray, data);
		QFETCH(VersionFileList, list);
		QFETCH(QString, error);
		QFETCH(bool, ret);

		VersionFileList outList;
		QString outError;
		bool outRet = parseVersionInfo(data, outList, outError);
		QCOMPARE(outRet, ret);
		QCOMPARE(outList, list);
		QCOMPARE(outError, error);
	}

	void test_processFileLists_data()
	{
		QTest::addColumn<QString>("tempFolder");
		QTest::addColumn<VersionFileList>("currentVersion");
		QTest::addColumn<VersionFileList>("newVersion");
		QTest::addColumn<OperationList>("expectedOperations");

		QTemporaryDir tempFolderObj;
		QString tempFolder = tempFolderObj.path();
		// update fileOne, keep fileTwo, remove fileThree
		QTest::newRow("test 1")
			<< tempFolder << (VersionFileList()
							  << VersionFileEntry{
									 "tests/data/fileOne", 493,
									 FileSourceList()
										 << FileSource(
												"http", "http://host/path/fileOne-1"),
									 "9eb84090956c484e32cb6c08455a667b"}
							  << VersionFileEntry{
									 "tests/data/fileTwo", 644,
									 FileSourceList()
										 << FileSource(
												"http", "http://host/path/fileTwo-1"),
									 "38f94f54fa3eb72b0ea836538c10b043"}
							  << VersionFileEntry{
									 "tests/data/fileThree", 420,
									 FileSourceList()
										 << FileSource(
												"http", "http://host/path/fileThree-1"),
									 "f12df554b21e320be6471d7154130e70"})
			<< (VersionFileList()
				<< VersionFileEntry{
					   "tests/data/fileOne", 493,
					   FileSourceList()
						   << FileSource("http",
															 "http://host/path/fileOne-2"),
					   "42915a71277c9016668cce7b82c6b577"}
				<< VersionFileEntry{
					   "tests/data/fileTwo", 644,
					   FileSourceList()
						   << FileSource("http",
															 "http://host/path/fileTwo-2"),
					   "38f94f54fa3eb72b0ea836538c10b043"})
			<< (OperationList()
				<< Operation::DeleteOp("tests/data/fileThree")
				<< Operation::CopyOp(
					   PathCombine(tempFolder,
								   QString("tests/data/fileOne").replace("/", "_")),
					   "tests/data/fileOne", 493));
	}
	void test_processFileLists()
	{
		QFETCH(QString, tempFolder);
		QFETCH(VersionFileList, currentVersion);
		QFETCH(VersionFileList, newVersion);
		QFETCH(OperationList, expectedOperations);

		OperationList operations;

		processFileLists(currentVersion, newVersion, QCoreApplication::applicationDirPath(), tempFolder, new NetJob("Dummy"), operations, false);
		qDebug() << (operations == expectedOperations);
		qDebug() << operations;
		qDebug() << expectedOperations;
		QCOMPARE(operations, expectedOperations);
	}
/*
	void test_masterTest()
	{
		qDebug() << "#####################";
		MMC->m_version.build = 1;
		MMC->m_version.channel = "develop";
		auto channels =
			QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/channels.json"));
		auto root = QUrl::fromLocalFile(QDir::current().absoluteFilePath("tests/data/"));
		qDebug() << "channels: " << channels;
		qDebug() << "root: " << root;
		MMC->updateChecker()->setChannelListUrl(channels.toString());
		MMC->updateChecker()->setCurrentChannel("develop");

		DownloadTask task(root.toString(), 2);

		QSignalSpy succeededSpy(&task, SIGNAL(succeeded()));

		task.start();

		QVERIFY(succeededSpy.wait());
	}
*/
	void test_OSXPathFixup()
	{
		QString path, pathOrig;
		bool result;
		// Proper OSX path
		pathOrig = path = "MultiMC.app/Foo/Bar/Baz";
		qDebug() << "Proper OSX path: " << path;
		result = fixPathForOSX(path);
		QCOMPARE(path, QString("Foo/Bar/Baz"));
		QCOMPARE(result, true);

		// Bad OSX path
		pathOrig = path = "translations/klingon.lol";
		qDebug() << "Bad OSX path: " << path;
		result = fixPathForOSX(path);
		QCOMPARE(path, pathOrig);
		QCOMPARE(result, false);
	}
};

extern "C"
{
	QTEST_GUILESS_MAIN(DownloadTaskTest)
}

#include "tst_DownloadTask.moc"
