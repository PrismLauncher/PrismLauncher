#include "JavaChecker.h"
#include <QFile>
#include <QProcess>

#define CHECKER_FILE "JavaChecker.jar"

JavaChecker::JavaChecker(QObject *parent) : QObject(parent)
{
}

int JavaChecker::performCheck(QString path)
{
	if(QFile::exists(CHECKER_FILE))
	{
		QFile::remove(CHECKER_FILE);
	}
	// extract the checker
	QFile(":/java/checker.jar").copy(CHECKER_FILE);

	QStringList args = {"-jar", CHECKER_FILE};

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

	if (status == QProcess::CrashExit || exitcode == 1)
	{
		emit checkFinished({});
		return;
	}

	QString p_stdout = _process->readAllStandardOutput();
	auto parts = p_stdout.split('=', QString::SkipEmptyParts);
	if (parts.size() != 2 || parts[0] != "os.arch")
	{
		emit checkFinished({});
		return;
	}

	auto os_arch = parts[1].remove('\n').remove('\r');
	bool is_64 = os_arch == "x86_64" || os_arch == "amd64";

	JavaCheckResult result;
	{
		result.valid = true;
		result.is_64bit = is_64;
		result.mojangPlatform = is_64 ? "64" : "32";
		result.realPlatform = os_arch;
	}
	emit checkFinished(result);
}

void JavaChecker::error(QProcess::ProcessError err)
{
	if(err == QProcess::FailedToStart)
	{
		killTimer.stop();
		emit checkFinished({});
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
