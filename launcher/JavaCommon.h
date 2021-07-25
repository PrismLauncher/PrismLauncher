#pragma once
#include <java/JavaChecker.h>

class QWidget;

/**
 * Common UI bits for the java pages to use.
 */
namespace JavaCommon
{
    bool checkJVMArgs(QString args, QWidget *parent);

    // Show a dialog saying that the Java binary was not usable
    void javaBinaryWasBad(QWidget *parent, JavaCheckResult result);
    // Show a dialog saying that the Java binary was not usable because of bad options
    void javaArgsWereBad(QWidget *parent, JavaCheckResult result);
    // Show a dialog saying that the Java binary was usable
    void javaWasOk(QWidget *parent, JavaCheckResult result);

    class TestCheck : public QObject
    {
        Q_OBJECT
    public:
        TestCheck(QWidget *parent, QString path, QString args, int minMem, int maxMem, int permGen)
            :m_parent(parent), m_path(path), m_args(args), m_minMem(minMem), m_maxMem(maxMem), m_permGen(permGen)
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
        QWidget *m_parent = nullptr;
        QString m_path;
        QString m_args;
        int m_minMem = 0;
        int m_maxMem = 0;
        int m_permGen = 64;
    };
}
