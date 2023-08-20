#pragma once
#include <QProcess>
#include <QTimer>
#include <memory>

#include "QObjectPtr.h"

#include "JavaVersion.h"

class JavaChecker;

struct JavaCheckResult {
    QString path;
    QString mojangPlatform;
    QString realPlatform;
    JavaVersion javaVersion;
    QString javaVendor;
    QString outLog;
    QString errorLog;
    bool is_64bit = false;
    int id;
    enum class Validity { Errored, ReturnedInvalidData, Valid } validity = Validity::Errored;
};

typedef shared_qobject_ptr<QProcess> QProcessPtr;
typedef shared_qobject_ptr<JavaChecker> JavaCheckerPtr;
class JavaChecker : public QObject {
    Q_OBJECT
   public:
    explicit JavaChecker(QObject* parent = 0);
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
   public slots:
    void timeout();
    void finished(int exitcode, QProcess::ExitStatus);
    void error(QProcess::ProcessError);
    void stdoutReady();
    void stderrReady();
};
