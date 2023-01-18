#include <QTest>
#include <QTimer>
#include <QThread>

#include <tasks/ConcurrentTask.h>
#include <tasks/MultipleOptionsTask.h>
#include <tasks/SequentialTask.h>
#include <tasks/Task.h>

/* Does nothing. Only used for testing. */
class BasicTask : public Task {
    Q_OBJECT

    friend class TaskTest;

   public:
    BasicTask(bool show_debug_log = true) : Task(nullptr, show_debug_log) {}

   private:
    void executeTask() override
    {
        emitSucceeded();
    };
};

/* Does nothing. Only used for testing. */
class BasicTask_MultiStep : public Task {
    Q_OBJECT

    friend class TaskTest;

   private:
    auto isMultiStep() const -> bool override { return true; }

    void executeTask() override {};   
};

class BigConcurrentTask : public QThread {
    Q_OBJECT

    ConcurrentTask big_task;

    void run() override
    {
        QTimer deadline;
        deadline.setInterval(10000);
        connect(&deadline, &QTimer::timeout, this, [this]{ passed_the_deadline = true; });
        deadline.start();

        static const unsigned s_num_tasks = 1 << 14;
        auto sub_tasks = new BasicTask*[s_num_tasks];

        for (unsigned i = 0; i < s_num_tasks; i++) {
            sub_tasks[i] = new BasicTask(false);
            big_task.addTask(sub_tasks[i]);
        }

        big_task.run();

        while (!big_task.isFinished() && !passed_the_deadline)
            QCoreApplication::processEvents();

        emit finished();
    }

   public:
    bool passed_the_deadline = false;

   signals:
    void finished();
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

    void test_basicRun(){
        BasicTask t;
        QObject::connect(&t, &Task::finished, [&]{ QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been."); });
        t.start();

        QVERIFY2(QTest::qWaitFor([&]() {
            return t.isFinished();
        }, 1000), "Task didn't finish as it should.");
    }

    void test_basicConcurrentRun(){
        BasicTask t1;
        BasicTask t2;
        BasicTask t3;

        ConcurrentTask t;

        t.addTask(&t1);
        t.addTask(&t2);
        t.addTask(&t3);

        QObject::connect(&t, &Task::finished, [&]{
                QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been.");
                QVERIFY(t1.wasSuccessful());
                QVERIFY(t2.wasSuccessful());
                QVERIFY(t3.wasSuccessful());
        });

        t.start();
        QVERIFY2(QTest::qWaitFor([&]() {
            return t.isFinished();
        }, 1000), "Task didn't finish as it should.");
    }

    // Tests if starting new tasks after the 6 initial ones is working
    void test_moreConcurrentRun(){
        BasicTask t1, t2, t3, t4, t5, t6, t7, t8, t9;

        ConcurrentTask t;

        t.addTask(&t1);
        t.addTask(&t2);
        t.addTask(&t3);
        t.addTask(&t4);
        t.addTask(&t5);
        t.addTask(&t6);
        t.addTask(&t7);
        t.addTask(&t8);
        t.addTask(&t9);

        QObject::connect(&t, &Task::finished, [&]{
                QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been.");
                QVERIFY(t1.wasSuccessful());
                QVERIFY(t2.wasSuccessful());
                QVERIFY(t3.wasSuccessful());
                QVERIFY(t4.wasSuccessful());
                QVERIFY(t5.wasSuccessful());
                QVERIFY(t6.wasSuccessful());
                QVERIFY(t7.wasSuccessful());
                QVERIFY(t8.wasSuccessful());
                QVERIFY(t9.wasSuccessful());
        });

        t.start();
        QVERIFY2(QTest::qWaitFor([&]() {
            return t.isFinished();
        }, 1000), "Task didn't finish as it should.");
    }

    void test_basicSequentialRun(){
        BasicTask t1;
        BasicTask t2;
        BasicTask t3;

        SequentialTask t;

        t.addTask(&t1);
        t.addTask(&t2);
        t.addTask(&t3);

        QObject::connect(&t, &Task::finished, [&]{
                QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been.");
                QVERIFY(t1.wasSuccessful());
                QVERIFY(t2.wasSuccessful());
                QVERIFY(t3.wasSuccessful());
        });

        t.start();
        QVERIFY2(QTest::qWaitFor([&]() {
            return t.isFinished();
        }, 1000), "Task didn't finish as it should.");
    }

    void test_basicMultipleOptionsRun(){
        BasicTask t1;
        BasicTask t2;
        BasicTask t3;

        MultipleOptionsTask t;

        t.addTask(&t1);
        t.addTask(&t2);
        t.addTask(&t3);

        QObject::connect(&t, &Task::finished, [&]{
                QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been.");
                QVERIFY(t1.wasSuccessful());
                QVERIFY(!t2.wasSuccessful());
                QVERIFY(!t3.wasSuccessful());
        });

        t.start();
        QVERIFY2(QTest::qWaitFor([&]() {
            return t.isFinished();
        }, 1000), "Task didn't finish as it should.");
    }

    void test_stackOverflowInConcurrentTask()
    {
        QEventLoop loop;

        auto thread = new BigConcurrentTask;
        thread->setStackSize(32 * 1024);

        connect(thread, &BigConcurrentTask::finished, &loop, &QEventLoop::quit);

        thread->start();

        loop.exec();

        QVERIFY(!thread->passed_the_deadline);
        thread->deleteLater();
    }
};

QTEST_GUILESS_MAIN(TaskTest)

#include "Task_test.moc"
