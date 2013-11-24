#pragma once
#include <QProcess>
#include <QTimer>
#include <memory>

struct JavaCheckResult
{
	QString mojangPlatform;
	QString realPlatform;
	bool valid = false;
	bool is_64bit = false;
};
typedef std::shared_ptr<QProcess> QProcessPtr;

class JavaChecker : public QObject
{
	Q_OBJECT
public:
	explicit JavaChecker(QObject *parent = 0);
	int performCheck(QString path);

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
