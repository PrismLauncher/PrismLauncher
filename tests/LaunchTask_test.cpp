#include <QTest>

#include <launch/LaunchStep.h>
#include <launch/LaunchStepIds.h>
#include <launch/LaunchTask.h>

#include <set>
#include <string_view>

namespace {

class DummyStep : public LaunchStep {
    Q_OBJECT
   public:
    explicit DummyStep(LaunchTask* parent) : LaunchStep(parent) {}

   protected:
    void executeTask() override { emitSucceeded(); }
};

// create() needs a real instance; the steps can be tested without one
class TestLaunchTask : public LaunchTask {
   public:
    TestLaunchTask() : LaunchTask(nullptr) {}

    shared_qobject_ptr<LaunchStep> step() { return makeShared<DummyStep>(this); }
};

}  // namespace

class LaunchTaskTest : public QObject {
    Q_OBJECT

   private slots:
    void test_appendAndPrependWithIds()
    {
        TestLaunchTask task;
        task.appendStep(task.step(), "b");
        task.appendStep(task.step());
        task.prependStep(task.step(), "a");
        QCOMPARE(task.stepIds(), QStringList({ "a", "b", "" }));
    }

    void test_insertBefore()
    {
        TestLaunchTask task;
        task.appendStep(task.step(), "java");
        task.appendStep(task.step(), "java");
        task.appendStep(task.step(), "launch");

        auto step = task.step();
        QVERIFY(task.insertStep(step, "java", LaunchTask::Position::Before));
        QCOMPARE(task.stepIds(), QStringList({ "", "java", "java", "launch" }));
        // The inserted step keeps its own id
        step->setId("mine");
        QCOMPARE(task.stepIds().first(), "mine");
    }

    void test_insertAfterLastOfGroup()
    {
        TestLaunchTask task;
        task.appendStep(task.step(), "java");
        task.appendStep(task.step(), "java");
        task.appendStep(task.step(), "launch");

        auto step = task.step();
        step->setId("mine");
        QVERIFY(task.insertStep(step, "java", LaunchTask::Position::After));
        QCOMPARE(task.stepIds(), QStringList({ "java", "java", "mine", "launch" }));

        auto last = task.step();
        last->setId("end");
        QVERIFY(task.insertStep(last, "launch", LaunchTask::Position::After));
        QCOMPARE(task.stepIds().last(), "end");
    }

    void test_insertUnknownAnchor()
    {
        TestLaunchTask task;
        task.appendStep(task.step(), "launch");
        QVERIFY(!task.insertStep(task.step(), "missing", LaunchTask::Position::Before));
        QVERIFY(!task.insertStep(task.step(), "", LaunchTask::Position::After));
        QCOMPARE(task.stepIds(), QStringList({ "launch" }));
    }

    void test_idsAreUnique()
    {
        std::set<std::string_view> seen;
        for (const char* id : LaunchStepIds::all) {
            QVERIFY2(seen.insert(id).second, id);
        }
        QCOMPARE(seen.size(), LaunchStepIds::all.size());
    }
};

QTEST_GUILESS_MAIN(LaunchTaskTest)

#include "LaunchTask_test.moc"
