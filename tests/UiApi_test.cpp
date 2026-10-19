#include <QAbstractItemView>
#include <QApplication>
#include <QCompleter>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

#include <plugins/PluginManifest.h>
#include <plugins/lua/LuaState.h>
#include <plugins/lua/PrismApi.h>
#include <ui/widgets/LuaCodeEditor.h>

#include <memory>

// Lua snippets avoid "//" and single quotes in raw strings, which confuse moc
class UiApiTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    std::unique_ptr<LuaState> newState(const QStringList& permissions = { "ui" })
    {
        PluginManifest manifest;
        manifest.id = "com.example.ui";
        manifest.permissions = permissions;
        auto state = std::make_unique<LuaState>();
        if (auto error = PrismApi::install(*state, manifest, m_dir.path())) {
            qFatal("install failed: %s", qPrintable(*error));
        }
        return state;
    }

    static void runOk(LuaState& state, const QByteArray& code)
    {
        auto error = state.run(code, "test");
        QVERIFY2(!error, qPrintable(error.value_or("")));
    }

    static void expectError(LuaState& state, const QByteArray& code, const QString& expected)
    {
        auto error = state.run(code, "test");
        QVERIFY2(error, "the code should have failed");
        QVERIFY2(error->contains(expected), qPrintable(*error));
    }

    static QDialog* findWindow(const QString& title)
    {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (auto* dialog = qobject_cast<QDialog*>(widget); dialog && dialog->windowTitle() == title) {
                return dialog;
            }
        }
        return nullptr;
    }

   private slots:
    void test_widgets()
    {
        auto state = newState();
        runOk(*state, R"lua(
            win = prism.ui.window{ title = "Test window", width = 600, height = 300 }
            local row = win:add("row")
            list = row:add("list", { items = { "a", "b", "c" }, onSelect = function(i, text) selected = i .. ":" .. text end })
            code = row:add("code", { text = "print(1)", stretch = 2 })
            input = win:add("input", { placeholder = "name" })
            label = win:add("label", { text = "Hello" })
            button = win:add("button", { text = "Go", onClick = function() clicks = (clicks or 0) + 1 end })
            win:show()
        )lua");

        QDialog* window = findWindow("Test window");
        QVERIFY(window);
        QVERIFY(window->isVisible());
        QCOMPARE(window->findChild<QListWidget*>()->count(), 3);

        auto* button = window->findChild<QPushButton*>();
        QTest::mouseClick(button, Qt::LeftButton);
        QTest::mouseClick(button, Qt::LeftButton);
        runOk(*state, "assert(clicks == 2, clicks)");

        runOk(*state, R"lua(
            list:select(2)
            assert(selected == "2:b", selected)
            assert(list:selected() == 2 and list:text() == "b")
            list:setItems({ "x", 7 })
            assert(list:selected() == nil)

            input:setText("Alex")
            assert(input:text() == "Alex")
            label:setText("Bye")
            assert(label:text() == "Bye")
            assert(code:text() == "print(1)")
            code:setProblems({ { line = 1, message = "oops" } })
            win:setText("Renamed")
        )lua");
        QCOMPARE(window->findChild<QLineEdit*>()->text(), "Alex");
        auto* editor = window->findChild<LuaCodeEditor*>();
        QCOMPARE(editor->problems().size(), 1);
        QCOMPARE(editor->problems().first().message, "oops");
        QCOMPARE(window->windowTitle(), "Renamed");

        // Typing in the editor calls onChange
        runOk(*state, "changes = 0");
        state->run("code2 = win:add(\"code\", { onChange = function() changes = changes + 1 end })", "test");
        auto editors = window->findChildren<LuaCodeEditor*>();
        QTest::keyClicks(editors.last(), "abc");
        runOk(*state, "assert(changes == 3, changes)");
    }

    void test_completion()
    {
        auto state = newState();
        runOk(*state, R"lua(
            win = prism.ui.window{ title = "Completion" }
            code = win:add("code", { onFocus = function(name) focused = name end })
            code:setCompletions({
                { name = "prism.log.info", signature = "prism.log.info(...)", doc = "Writes to the log." },
                { name = "prism.log.warn", signature = "prism.log.warn(...)" },
                { name = "string.format" },
            })
            code:setText("local counter = 1\n")
            win:show()
        )lua");
        auto* editor = findWindow("Completion")->findChild<LuaCodeEditor*>();
        QCOMPARE(editor->completionsFor("prism.log."), QStringList({ "prism.log.info", "prism.log.warn" }));
        // Names used in the text and keywords are offered too
        QVERIFY(editor->completionsFor("cou").contains("counter"));
        QVERIFY(editor->completionsFor("loc").contains("local"));

        // Typing shows the popup; Enter inserts the highlighted completion
        editor->moveCursor(QTextCursor::End);
        QTest::keyClicks(editor, "prism.log.i");
        QVERIFY(editor->completer()->popup()->isVisible());
        QTest::keyClick(editor->completer()->popup(), Qt::Key_Return);
        QVERIFY2(editor->toPlainText().endsWith("prism.log.info"), qPrintable(editor->toPlainText()));
        runOk(*state, "assert(focused == \"prism.log.info\", focused)");

        // Ctrl+Space shows it without typing two characters
        QTest::keyClick(editor, Qt::Key_Return);
        QTest::keyClicks(editor, "s");
        QTest::keyClick(editor, Qt::Key_Space, Qt::ControlModifier);
        QVERIFY(editor->completer()->popup()->isVisible());
        QTest::keyClick(editor->completer()->popup(), Qt::Key_Escape);
        QVERIFY(!editor->completer()->popup()->isVisible());

        // Moving the cursor reports the name under it
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(8);
        editor->setTextCursor(cursor);
        runOk(*state, "assert(focused == \"counter\", focused)");

        expectError(*state, "code:setCompletions({ { doc = \"x\" } })", "completions need a name");
        expectError(*state, "code:setCompletions({ { name = 1 } })", "'name' must be a string");
        expectError(*state, "win:add(\"label\"):setCompletions({})", "a label has no method 'setCompletions'");
    }

    void test_closeAndUnload()
    {
        auto state = newState();
        runOk(*state, R"lua(
            win = prism.ui.window{ title = "Closing", onClose = function() closed = true end }
            label = win:add("label", { text = "x" })
            win:show()
            win:close()
        )lua");
        runOk(*state, "assert(closed)");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!findWindow("Closing"));
        expectError(*state, "label:setText(\"y\")", "the window was closed");

        runOk(*state, "other = prism.ui.window{ title = \"Unloaded\" } other:show()");
        QPointer<QDialog> window = findWindow("Unloaded");
        QVERIFY(window);
        // Destroying the state closes its windows
        state.reset();
        QVERIFY(window.isNull());
    }

    void test_errors()
    {
        auto state = newState();
        runOk(*state, "win = prism.ui.window{ title = \"Errors\" } label = win:add(\"label\")");
        expectError(*state, "win:add(\"slider\")", "unknown widget kind 'slider'");
        expectError(*state, "label:add(\"label\")", "a label cannot contain widgets");
        expectError(*state, "label:setItems({})", "a label has no method 'setItems'");
        expectError(*state, "win:add(\"label\", { text = 5 })", "'text' must be a string");
        expectError(*state, "win:add(\"button\", { onClick = 5 })", "'onClick' must be a function");

        auto denied = newState({});
        expectError(*denied, "prism.ui.window{}", "prism.ui.window requires the 'ui' permission");
    }

    void test_callbackErrorsAreLogged()
    {
        auto state = newState();
        runOk(*state, R"lua(
            win = prism.ui.window{ title = "Failing" }
            win:add("button", { text = "Fail", onClick = function() error("from a button") end })
        )lua");
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Plugin window callback failed: .*from a button"));
        QTest::mouseClick(findWindow("Failing")->findChild<QPushButton*>(), Qt::LeftButton);
    }

    void test_windowLimit()
    {
        auto state = newState();
        expectError(*state, "for i = 1, 9 do prism.ui.window{} end", "too many open windows, the maximum is 8");
    }
};

int main(int argc, char* argv[])
{
    // No real screen needed
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    UiApiTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "UiApi_test.moc"
