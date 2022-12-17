#pragma once
#include <QProcess>
#include <QTimer>
#include <memory>

#include "QObjectPtr.h"

#include "JavaVersion.h"

class JavaChecker;

struct JavaCheckResult
{
    QString path;
    QString mojangPlatform;
    QString realPlatform;
    JavaVersion javaVersion;
    QString javaVendor;
    QString outLog;
    QString errorLog;
    bool is_64bit = false;
    int id;
    enum class Validity
    {
        Errored,
        ReturnedInvalidData,
        Valid
    } validity = Validity::Errored;
};

typedef shared_qobject_ptr<QProcess> QProcessPtr;
typedef shared_qobject_ptr<JavaChecker> JavaCheckerPtr;
class JavaChecker : public QObject
{
    Q_OBJECT
public:
    explicit JavaChecker(QObject *parent = 0);
    void performCheck();

    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_args;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_id = 0;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMem = 0;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMem = 0;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGen = 64;

signals:
    void checkFinished(JavaCheckResult result);
private:
    QProcessPtr process;
    QTimer killTimer;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stdout;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stderr;
public
slots:
    void timeout();
    void finished(int exitcode, QProcess::ExitStatus);
    void error(QProcess::ProcessError);
    void stdoutReady();
    void stderrReady();
};
