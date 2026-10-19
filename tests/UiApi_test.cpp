#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QTextBlock>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

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

    void test_split()
    {
        auto state = newState();
        runOk(*state, R"lua(
            win = prism.ui.window{ title = "Split", width = 900, height = 500 }
            split = win:add("split", { stretch = 1 })
            left = split:add("list")
            middle = split:add("code", { stretch = 2 })
            right = split:add("column")
            right:add("label", { text = "help" })
            win:show()
            split:setSizes({ 200, 500, 200 })
        )lua");
        QDialog* window = findWindow("Split");
        auto* splitter = window->findChild<QSplitter*>();
        QVERIFY(splitter);
        QCOMPARE(splitter->count(), 3);
        QCOMPARE(splitter->orientation(), Qt::Horizontal);
        runOk(*state, R"lua(
            local sizes = split:sizes()
            assert(#sizes == 3 and sizes[1] > 0 and sizes[2] > sizes[1], table.concat(sizes, ","))
            local w, h = win:size()
            assert(w > 0 and h > 0)
            right:setVisible(false)
        )lua");
        QVERIFY(splitter->widget(2)->isHidden());
        runOk(*state, R"lua(
            right:setVisible(true)
            local vertical = win:add("split", { vertical = true })
            vertical:add("label", { text = "top" })
        )lua");
        QVERIFY(!splitter->widget(2)->isHidden());
        QCOMPARE(window->findChildren<QSplitter*>().last()->orientation(), Qt::Vertical);
        expectError(*state, "left:sizes()", "a list has no method 'sizes'");
        expectError(*state, "split:setSizes(5)", "table expected");
    }

    void test_comboAndTree()
    {
        auto state = newState();
        runOk(*state, R"lua(
            win = prism.ui.window{ title = "Pickers" }
            combo = win:add("combo", { items = { "alpha", "beta" }, onSelect = function(i, text) picked = i .. ":" .. text end })
            tree = win:add("tree", { onSelect = function(id, text) chosen = id .. "|" .. text end })
            tree:setItems({
                { text = "src", children = {
                    { text = "util", children = { { text = "deep.lua", id = "src/util/deep.lua" } } },
                    { text = "a.lua", id = "src/a.lua" },
                } },
                { text = "main.lua", id = "main.lua" },
            })
            win:show()
        )lua");
        QDialog* window = findWindow("Pickers");
        auto* comboBox = window->findChild<QComboBox*>();
        auto* treeWidget = window->findChild<QTreeWidget*>();
        QCOMPARE(comboBox->count(), 2);
        QCOMPARE(treeWidget->topLevelItemCount(), 2);
        QCOMPARE(treeWidget->topLevelItem(0)->childCount(), 2);

        runOk(*state, R"lua(
            assert(combo:selected() == 1 and combo:text() == "alpha")
            combo:select(2)
            assert(picked == "2:beta", picked)
            assert(combo:selected() == 2)

            assert(tree:selected() == nil)
            tree:select("src/util/deep.lua")
            assert(chosen == "src/util/deep.lua|deep.lua", chosen)
            assert(tree:selected() == "src/util/deep.lua" and tree:text() == "deep.lua")
        )lua");
        // Selecting opened the folders above it
        QVERIFY(treeWidget->topLevelItem(0)->isExpanded());
        QVERIFY(treeWidget->topLevelItem(0)->child(0)->isExpanded());

        // Folders have no id
        treeWidget->setCurrentItem(treeWidget->topLevelItem(0));
        runOk(*state, "assert(chosen == \"|src\", chosen) assert(tree:selected() == nil)");

        expectError(*state, "tree:setItems({ { id = \"x\" } })", "tree items need a text");
        expectError(*state, "tree:setItems({ { text = \"x\", id = 1 } })", "'id' must be a string");
        expectError(*state, "win:add(\"label\"):select(1)", "a label has no method 'select'");
    }

    void test_lineClipboard()
    {
        auto state = newState();
        runOk(*state, "win = prism.ui.window{ title = \"Lines\" } code = win:add(\"code\", { text = \"one\\ntwo\\nthree\" }) win:show()");
        auto* editor = findWindow("Lines")->findChild<LuaCodeEditor*>();
        auto moveTo = [editor](int line, int column) {
            QTextCursor cursor(editor->document()->findBlockByNumber(line));
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
            editor->setTextCursor(cursor);
        };

        // Copy without a selection takes the whole line; pasting it goes below the current one
        moveTo(1, 1);
        QTest::keyClick(editor, Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(QApplication::clipboard()->text(), "two\n");
        moveTo(0, 1);
        QTest::keyClick(editor, Qt::Key_V, Qt::ControlModifier);
        QCOMPARE(editor->toPlainText(), "one\ntwo\ntwo\nthree");

        // Cut without a selection removes the whole line
        moveTo(0, 2);
        QTest::keyClick(editor, Qt::Key_X, Qt::ControlModifier);
        QCOMPARE(editor->toPlainText(), "two\ntwo\nthree");
        QCOMPARE(QApplication::clipboard()->text(), "one\n");
        // Also the last one, with the line break before it
        moveTo(2, 0);
        QTest::keyClick(editor, Qt::Key_X, Qt::ControlModifier);
        QCOMPARE(editor->toPlainText(), "two\ntwo");

        // With a selection, copy and paste work as usual
        QTextCursor cursor(editor->document()->findBlockByNumber(0));
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        editor->setTextCursor(cursor);
        QTest::keyClick(editor, Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(QApplication::clipboard()->text(), "tw");
        moveTo(1, 3);
        QTest::keyClick(editor, Qt::Key_V, Qt::ControlModifier);
        QCOMPARE(editor->toPlainText(), "two\ntwotw");
    }

    void test_shortcuts()
    {
        auto state = newState();
        runOk(*state, R"lua(
            win = prism.ui.window{ title = "Shortcuts" }
            win:add("code")
            win:addShortcut("Ctrl+S", function() saves = (saves or 0) + 1 end)
            win:show()
        )lua");
        QDialog* window = findWindow("Shortcuts");
        window->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(window));
        QTest::keyClick(window->findChild<LuaCodeEditor*>(), Qt::Key_S, Qt::ControlModifier);
        runOk(*state, "assert(saves == 1, saves)");

        expectError(*state, "win:addShortcut(\"Ctrl+Nope+Q\", function() end)", "invalid shortcut");
        expectError(*state, "win:addShortcut(\"Ctrl+S\")", "function expected");
        expectError(*state, "win:add(\"label\"):addShortcut(\"Ctrl+S\", function() end)", "a label has no method 'addShortcut'");
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
