#include "JavaChecker.h"
#include <QFile>
#include <QProcess>
#include <QMap>
#include <QTemporaryFile>

#define CHECKER_FILE "JavaChecker.jar"

JavaChecker::JavaChecker(QObject *parent) : QObject(parent)
{
}

void JavaChecker::performCheck()
{
	checkerJar.setFileTemplate("checker_XXXXXX.jar");
	checkerJar.open();
	QFile inner(":/java/checker.jar");
	inner.open(QIODevice::ReadOnly);
	checkerJar.write(inner.readAll());
	inner.close();
	checkerJar.close();

	QStringList args = {"-jar", checkerJar.fileName()};

	process.reset(new QProcess());
	process->setArguments(args);
	process->setProgram(path);
	process->setProcessChannelMode(QProcess::SeparateChannels);

	connect(process.get(), SIGNAL(finished(int, QProcess::ExitStatus)), this,
			SLOT(finished(int, QProcess::ExitStatus)));
	connect(process.get(), SIGNAL(error(QProcess::ProcessError)), this,
			SLOT(error(QProcess::ProcessError)));
	connect(&killTimer, SIGNAL(timeout()), SLOT(timeout()));
	killTimer.setSingleShot(true);
	killTimer.start(5000);
	process->start();
}

void JavaChecker::finished(int exitcode, QProcess::ExitStatus status)
{
	killTimer.stop();
	QProcessPtr _process;
	_process.swap(process);
	checkerJar.remove();

	JavaCheckResult result;
	{
		result.path = path;
	}

	if (status == QProcess::CrashExit || exitcode == 1)
	{
		emit checkFinished(result);
		return;
	}

	bool success = true;
	QString p_stdout = _process->readAllStandardOutput();
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

	emit checkFinished(result);
}

void JavaChecker::error(QProcess::ProcessError err)
{
	if(err == QProcess::FailedToStart)
	{
		killTimer.stop();

		JavaCheckResult result;
		{
			result.path = path;
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
		process->kill();
		process.reset();
	}
}
