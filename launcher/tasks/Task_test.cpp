#include <QTest>

#include "Task.h"

/* Does nothing. Only used for testing. */
class BasicTask : public Task {
    Q_OBJECT

    friend class TaskTest;

   private:
    void executeTask() override {};   
};

/* Does nothing. Only used for testing. */
class BasicTask_MultiStep : public Task {
    Q_OBJECT

    friend class TaskTest;

   private:
    auto isMultiStep() const -> bool override { return true; }

    void executeTask() override {};   
};

class TaskTest : public QObject {
    Q_OBJECT

   private slots:
    void test_SetStatus_NoMultiStep(){
        BasicTask t;
        QString status {"test status"};

        t.setStatus(status);

        QCOMPARE(t.getStatus(), status);
        QCOMPARE(t.getStepStatus(), status);
    }

    void test_SetStatus_MultiStep(){
        BasicTask_MultiStep t;
        QString status {"test status"};

        t.setStatus(status);

        QCOMPARE(t.getStatus(), status);
        // Even though it is multi step, it does not override the getStepStatus method,
        // so it should remain the same.
        QCOMPARE(t.getStepStatus(), status);
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
