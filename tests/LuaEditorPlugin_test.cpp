#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

#include <FileSystem.h>
#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
#include <plugins/lua/LuaState.h>
#include <settings/INISettingsObject.h>
#include <settings/Setting.h>
#include <ui/widgets/LuaCodeEditor.h>
#include <algorithm>
#include <functional>
#include <memory>

// Drives the example editor plugin (examples/plugins/prism.luaeditor) through its window
class LuaEditorPluginTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_root;
    std::unique_ptr<INISettingsObject> m_settings;
    std::unique_ptr<PluginManager> m_manager;

    QString pluginsDir() const { return m_root.filePath("plugins"); }

    static QDialog* editorWindow()
    {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (auto* dialog = qobject_cast<QDialog*>(widget);
                dialog && dialog->windowTitle().startsWith("Lua editor") && dialog->isVisible()) {
                return dialog;
            }
        }
        return nullptr;
    }

    // Ids of the files in the tree, with "/" separating folders as shown
    static QStringList treeEntries(QDialog* window)
    {
        QStringList entries;
        std::function<void(QTreeWidgetItem*, const QString&)> walk = [&](QTreeWidgetItem* item, const QString& prefix) {
            const QString name = prefix + item->text(0);
            entries.append(item->childCount() > 0 ? name + "/" : name);
            for (int i = 0; i < item->childCount(); i++) {
                walk(item->child(i), name + "/");
            }
        };
        auto* tree = window->findChild<QTreeWidget*>();
        for (int i = 0; i < tree->topLevelItemCount(); i++) {
            walk(tree->topLevelItem(i), "");
        }
        return entries;
    }

    static void openFile(QDialog* window, const QString& id)
    {
        auto* tree = window->findChild<QTreeWidget*>();
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->data(0, Qt::UserRole).toString() == id) {
                tree->setCurrentItem(*it);
                return;
            }
        }
        QFAIL(qPrintable("no " + id + " in the tree"));
    }

    static QString openedFile(QDialog* window)
    {
        QTreeWidgetItem* item = window->findChild<QTreeWidget*>()->currentItem();
        return item ? item->data(0, Qt::UserRole).toString() : QString();
    }

    // The status line above the code, not the documentation's title
    static QLabel* statusLabel(QDialog* window)
    {
        for (auto* label : window->findChildren<QLabel*>()) {
            if (!label->text().startsWith("Documentation:")) {
                return label;
            }
        }
        return nullptr;
    }

    static QPushButton* button(QDialog* window, const QString& text)
    {
        for (auto* button : window->findChildren<QPushButton*>()) {
            if (button->text() == text) {
                return button;
            }
        }
        return nullptr;
    }

   private slots:
    void initTestCase()
    {
        QVERIFY(m_root.isValid());
        QVERIFY(FS::copy(QString(EXAMPLES_DIR) + "/prism.luaeditor", pluginsDir() + "/prism.luaeditor")());
        QDir dir(pluginsDir());
        QVERIFY(dir.mkpath("com.example.target"));
        QFile toml(dir.filePath("com.example.target/plugin.toml"));
        QVERIFY(toml.open(QIODevice::WriteOnly));
        toml.write("id = \"com.example.target\"\nname = \"Target\"\nversion = \"1\"\napi = 1\n");
        toml.close();
        QFile main(dir.filePath("com.example.target/main.lua"));
        QVERIFY(main.open(QIODevice::WriteOnly));
        main.write("prism.log.info(\"hello\")\n");
        main.close();
        // A module in a folder, and files the editor does not show
        QVERIFY(dir.mkpath("com.example.target/lib"));
        QVERIFY(dir.mkpath("com.example.target/assets"));
        for (const char* path :
             { "com.example.target/lib/util.lua", "com.example.target/icon.png", "com.example.target/assets/logo.png" }) {
            QFile file(dir.filePath(path));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("return {}\n");
        }

        m_settings = std::make_unique<INISettingsObject>(m_root.filePath("settings.cfg"));
        auto enabled = m_settings->registerSetting("EnabledPlugins", QStringList({ "prism.luaeditor" }));
        m_manager = std::make_unique<PluginManager>(pluginsDir(), enabled);
        m_manager->discover();
        QVERIFY2(m_manager->plugin("prism.luaeditor")->state() == Plugin::State::Loaded,
                 qPrintable(m_manager->plugin("prism.luaeditor")->error()));
    }

    void test_everyApiIsDocumented()
    {
        // Runs in the editor's own state, where require finds its docs module
        LuaState* lua = m_manager->plugin("prism.luaeditor")->lua();
        auto error = lua->run(
            "local known = {}\n"
            "for _, entry in ipairs(require(\"docs\")) do known[entry.name] = true end\n"
            "local missing = {}\n"
            "local function walk(value, name, seen)\n"
            "    if not known[name] then missing[#missing + 1] = name end\n"
            "    if type(value) ~= \"table\" or seen[value] then return end\n"
            "    seen[value] = true\n"
            "    for key, child in pairs(value) do\n"
            "        if type(key) == \"string\" then walk(child, name .. \".\" .. key, seen) end\n"
            "    end\n"
            "end\n"
            "walk(prism, \"prism\", {})\n"
            "table.sort(missing)\n"
            "assert(#missing == 0, \"not documented in docs.lua: \" .. table.concat(missing, \", \"))\n",
            "coverage");
        QVERIFY2(!error, qPrintable(error.value_or("")));
    }

    void test_editAndCheck()
    {
        const auto actions = m_manager->actions(PrismApi::ActionScope::Launcher);
        QCOMPARE(actions.size(), 1);
        QCOMPARE(actions.first().text, "Lua editor");
        QCOMPARE(m_manager->runAction(actions.first()), std::nullopt);

        QDialog* window = editorWindow();
        QVERIFY(window);

        // A plugin is picked first; only its files show, in their folders
        auto* picker = window->findChild<QComboBox*>();
        QStringList plugins;
        for (int i = 0; i < picker->count(); i++) {
            plugins.append(picker->itemText(i));
        }
        QCOMPARE(plugins, QStringList({ "com.example.target", "prism.luaeditor" }));
        QCOMPARE(picker->currentText(), "com.example.target");
        QCOMPARE(treeEntries(window), QStringList({ "lib/", "lib/util.lua", "main.lua", "plugin.toml" }));

        openFile(window, "com.example.target/main.lua");
        auto* editor = window->findChild<LuaCodeEditor*>();
        QCOMPARE(editor->toPlainText(), "prism.log.info(\"hello\")\n");
        QVERIFY(editor->problems().isEmpty());

        // A typo in the API and a syntax error, while typing
        editor->setPlainText("prism.log.inf(\"x\")\nprism.fs.dta():read(\"a\")\nlocal s = \"prism.nothing\" -- prism.nope\nif then\n");
        const auto problems = editor->problems();
        QStringList messages;
        for (const auto& problem : problems) {
            messages.append(QString("%1: %2").arg(problem.line).arg(problem.message));
        }
        QVERIFY2(messages.contains("1: unknown API prism.log.inf, did you mean prism.log.info?"), qPrintable(messages.join("\n")));
        QVERIFY2(messages.contains("2: unknown API prism.fs.dta, did you mean prism.fs.data?"), qPrintable(messages.join("\n")));
        QVERIFY2(std::any_of(messages.begin(), messages.end(), [](const QString& m) { return m.startsWith("4: "); }),
                 qPrintable(messages.join("\n")));
        // Names inside strings and comments are not checked
        QCOMPARE(problems.size(), 3);
        QVERIFY(window->windowTitle().endsWith("*"));

        // Cannot switch files or plugins with unsaved changes
        openFile(window, "com.example.target/lib/util.lua");
        QCOMPARE(openedFile(window), "com.example.target/main.lua");
        QVERIFY(statusLabel(window)->text().contains("Save or revert"));
        picker->setCurrentText("prism.luaeditor");
        QCOMPARE(picker->currentText(), "com.example.target");

        // Saving writes the file
        editor->setPlainText("prism.log.info(\"saved\")\n");
        // With the shortcut
        window->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(window));
        QTest::keyClick(editor, Qt::Key_S, Qt::ControlModifier);
        QVERIFY2(statusLabel(window)->text().contains("Saved"), qPrintable(statusLabel(window)->text()));
        QFile saved(pluginsDir() + "/com.example.target/main.lua");
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), "prism.log.info(\"saved\")\n");
        QVERIFY(!window->windowTitle().endsWith("*"));

        // Revert goes back to the saved text
        editor->setPlainText("changed");
        QTest::mouseClick(button(window, "Revert"), Qt::LeftButton);
        QCOMPARE(editor->toPlainText(), "prism.log.info(\"saved\")\n");

        // Another plugin: its files, and nothing open
        picker->setCurrentText("prism.luaeditor");
        QCOMPARE(treeEntries(window), QStringList({ "checks.lua", "docs.lua", "main.lua", "plugin.toml" }));
        QVERIFY(editor->toPlainText().isEmpty());
        openFile(window, "prism.luaeditor/checks.lua");
        QVERIFY(editor->toPlainText().contains("Finds problems in Lua code"));

        // Manifests open too, without Lua checks
        picker->setCurrentText("com.example.target");
        openFile(window, "com.example.target/plugin.toml");
        QVERIFY(editor->toPlainText().startsWith("id = \"com.example.target\""));
        QVERIFY(editor->problems().isEmpty());
        QVERIFY(statusLabel(window)->text().contains("need approval again"));

        // Completion offers the API and Lua
        QVERIFY(editor->completionsFor("prism.fs.").contains("prism.fs.data"));
        QVERIFY(editor->completionsFor("string.f").contains("string.format"));
        QVERIFY(!editor->completionsFor("folder").contains("folder:read"));

        // The documentation follows the cursor
        QPlainTextEdit* docView = nullptr;
        for (auto* edit : window->findChildren<QPlainTextEdit*>()) {
            if (!qobject_cast<LuaCodeEditor*>(edit)) {
                docView = edit;
            }
        }
        QVERIFY(docView);
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(8);  // inside prism.log.info
        editor->setTextCursor(cursor);
        QVERIFY2(docView->toPlainText().startsWith("prism.log.info(...)"), qPrintable(docView->toPlainText()));

        // Searching, by name and by description
        auto* results = window->findChild<QListWidget*>();
        QTest::keyClicks(window->findChild<QLineEdit*>(), "http");
        QStringList found;
        for (int i = 0; i < results->count(); i++) {
            found.append(results->item(i)->text());
        }
        QVERIFY2(found.contains("prism.http.get") && found.contains("prism.async"), qPrintable(found.join(", ")));
        results->setCurrentRow(static_cast<int>(found.indexOf("prism.http.get")));
        QVERIFY(docView->toPlainText().contains("Needs the net permission"));

        // The panes can be resized and the help hidden
        QVERIFY(window->findChild<QSplitter*>());
        QTest::mouseClick(button(window, "Hide help"), Qt::LeftButton);
        QVERIFY(!docView->isVisible());
        QVERIFY(button(window, "Show help"));
        QTest::mouseClick(button(window, "Show help"), Qt::LeftButton);
        QVERIFY(docView->isVisible());

        // Closing remembers the layout; reopening restores it, also with the help hidden
        QTest::mouseClick(button(window, "Hide help"), Qt::LeftButton);
        window->resize(1111, 666);
        window->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        // Plugin settings are written once control returns to the event loop
        QCoreApplication::processEvents();
        QFile settings(pluginsDir() + "/prism.luaeditor/settings.ini");
        QVERIFY(settings.open(QIODevice::ReadOnly));
        const QByteArray layout = settings.readAll();
        QVERIFY2(layout.contains("showHelp=false") && layout.contains("width=1111"), layout.constData());
        settings.close();

        QCOMPARE(m_manager->runAction(m_manager->actions(PrismApi::ActionScope::Launcher).first()), std::nullopt);
        QDialog* reopened = editorWindow();
        QVERIFY(reopened);
        QCOMPARE(reopened->size(), QSize(1111, 666));
        QCOMPARE(reopened->findChild<QComboBox*>()->currentText(), "com.example.target");
        QVERIFY(button(reopened, "Show help"));
        reopened->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    LuaEditorPluginTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "LuaEditorPlugin_test.moc"
