#include <QTest>
#include "TestUtil.h"

#include "Task.h"

/* Does nothing. Only used for testing. */
class BasicTask : public Task {
    Q_OBJECT
   public:
    explicit BasicTask() : Task() {};
   private:
    void executeTask() override {};   
};

class TaskTest : public QObject {
    Q_OBJECT

   private slots:
    void test_SetStatus(){
        BasicTask t;
        QString status {"test status"};

        t.setStatus(status);

        QCOMPARE(t.getStatus(), status);
    }

    void test_SetProgress(){
        BasicTask t;
        int current = 42;
        int total = 207;

        t.setProgress(current, total);

        QCOMPARE(t.getProgress(), current);
        QCOMPARE(t.getTotalProgress(), total);
    }
};

QTEST_GUILESS_MAIN(TaskTest)

#include "Task_test.moc"
