#pragma once
#include <QProcess>
#include <QTimer>
#include <memory>

class JavaChecker;


struct JavaCheckResult
{
	QString path;
	QString mojangPlatform;
	QString realPlatform;
	QString javaVersion;
	QString stderr;
	bool valid = false;
	bool is_64bit = false;
	int id;
};

typedef std::shared_ptr<QProcess> QProcessPtr;
typedef std::shared_ptr<JavaChecker> JavaCheckerPtr;
class JavaChecker : public QObject
{
	Q_OBJECT
public:
	explicit JavaChecker(QObject *parent = 0);
	void performCheck();

	QString m_path;
	QString m_args;
	int m_id = 0;
	int m_minMem = 0;
	int m_maxMem = 0;
	int m_permGen = 64;

signals:
	void checkFinished(JavaCheckResult result);
private:
	QProcessPtr process;
	QTimer killTimer;
	QString m_stdout;
	QString m_stderr;
public
slots:
	void timeout();
	void finished(int exitcode, QProcess::ExitStatus);
	void error(QProcess::ProcessError);
	void stdoutReady();
	void stderrReady();
};
