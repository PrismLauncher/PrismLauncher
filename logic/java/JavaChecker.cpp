#include "JavaChecker.h"
#include "MultiMC.h"
#include <pathutils.h>
#include <QFile>
#include <QProcess>
#include <QMap>
#include <QTemporaryFile>

JavaChecker::JavaChecker(QObject *parent) : QObject(parent)
{
}

void JavaChecker::performCheck()
{
	QString checkerJar = PathCombine(MMC->bin(), "jars", "JavaCheck.jar");

	QStringList args = {"-jar", checkerJar};

	process.reset(new QProcess());
	process->setArguments(args);
	process->setProgram(path);
	process->setProcessChannelMode(QProcess::SeparateChannels);
	QLOG_DEBUG() << "Running java checker!";
	QLOG_DEBUG() << "Java: " + path;
	QLOG_DEBUG() << "Args: {" + args.join("|") + "}";

	connect(process.get(), SIGNAL(finished(int, QProcess::ExitStatus)), this,
			SLOT(finished(int, QProcess::ExitStatus)));
	connect(process.get(), SIGNAL(error(QProcess::ProcessError)), this,
			SLOT(error(QProcess::ProcessError)));
	connect(&killTimer, SIGNAL(timeout()), SLOT(timeout()));
	killTimer.setSingleShot(true);
	killTimer.start(15000);
	process->start();
}

void JavaChecker::finished(int exitcode, QProcess::ExitStatus status)
{
	killTimer.stop();
	QProcessPtr _process;
	_process.swap(process);

	JavaCheckResult result;
	{
		result.path = path;
		result.id = id;
	}
	QLOG_DEBUG() << "Java checker finished with status " << status << " exit code " << exitcode;

	if (status == QProcess::CrashExit || exitcode == 1)
	{
		QLOG_DEBUG() << "Java checker failed!";
		emit checkFinished(result);
		return;
	}

	bool success = true;
	QString p_stdout = _process->readAllStandardOutput();
	QLOG_DEBUG() << p_stdout;

	QMap<QString, QString> results;
	QStringList lines = p_stdout.split("\n", QString::SkipEmptyParts);
	for(QString line : lines)
	{
		line = line.trimmed();

		auto parts = line.split('=', QString::SkipEmptyParts);
		if(parts.size() != 2 || parts[0].isEmpty() || parts[1].isEmpty())
		{
			success = false;
		}
		else
		{
			results.insert(parts[0], parts[1]);
		}
	}

	if(!results.contains("os.arch") || !results.contains("java.version") || !success)
	{
		QLOG_DEBUG() << "Java checker failed - couldn't extract required information.";
		emit checkFinished(result);
		return;
	}

	auto os_arch = results["os.arch"];
	auto java_version = results["java.version"];
	bool is_64 = os_arch == "x86_64" || os_arch == "amd64";


	result.valid = true;
	result.is_64bit = is_64;
	result.mojangPlatform = is_64 ? "64" : "32";
	result.realPlatform = os_arch;
	result.javaVersion = java_version;
	QLOG_DEBUG() << "Java checker succeeded.";
	emit checkFinished(result);
}

void JavaChecker::error(QProcess::ProcessError err)
{
	if(err == QProcess::FailedToStart)
	{
		killTimer.stop();
		QLOG_DEBUG() << "Java checker has failed to start.";
		JavaCheckResult result;
		{
			result.path = path;
			result.id = id;
		}

		emit checkFinished(result);
		return;
	}
}

void JavaChecker::timeout()
{
	// NO MERCY. NO ABUSE.
	if(process)
	{
		QLOG_DEBUG() << "Java checker has been killed by timeout.";
		process->kill();
	}
}
