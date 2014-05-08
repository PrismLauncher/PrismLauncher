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

	QString path;
	int id;

signals:
	void checkFinished(JavaCheckResult result);
private:
	QProcessPtr process;
	QTimer killTimer;
public
slots:
	void timeout();
	void finished(int exitcode, QProcess::ExitStatus);
	void error(QProcess::ProcessError);
};
