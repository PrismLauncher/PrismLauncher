#pragma once
#include <java/JavaChecker.h>

class QWidget;

/**
 * Common UI bits for the java pages to use.
 */
namespace JavaCommon
{
    bool checkJVMArgs(QString args, QWidget *parent);

    // Show a dialog saying that the Java binary was usable
    void javaWasOk(QWidget *parent, JavaCheckResult result);
    // Show a dialog saying that the Java binary was not usable because of bad options
    void javaArgsWereBad(QWidget *parent, JavaCheckResult result);
    // Show a dialog saying that the Java binary was not usable
    void javaBinaryWasBad(QWidget *parent, JavaCheckResult result);
    // Show a dialog if we couldn't find Java Checker
    void javaCheckNotFound(QWidget *parent);

    class TestCheck : public QObject
    {
        Q_OBJECT
    public:
        TestCheck(QWidget *parent, QString path, QString args, int minMem, int maxMem, int permGen)
            :hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path(path), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_args(args), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMem(minMem), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMem(maxMem), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGen(permGen)
        {
        }
        virtual ~TestCheck() {};

        void run();

    signals:
        void finished();

    private slots:
        void checkFinished(JavaCheckResult result);
        void checkFinishedWithArgs(JavaCheckResult result);

    private:
        std::shared_ptr<JavaChecker> checker;
        QWidget *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent = nullptr;
        QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path;
        QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_args;
        int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMem = 0;
        int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMem = 0;
        int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGen = 64;
    };
}
