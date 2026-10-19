#include <QApplication>
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

#include <FileSystem.h>
#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
#include <plugins/lua/LuaState.h>
#include <settings/INISettingsObject.h>
#include <settings/Setting.h>
#include <ui/widgets/LuaCodeEditor.h>
#include <algorithm>
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

    // The list of files: its entries are paths inside the plugins folder
    static QListWidget* filesList(QDialog* window)
    {
        for (auto* list : window->findChildren<QListWidget*>()) {
            if (list->count() > 0 && list->item(0)->text().contains('/')) {
                return list;
            }
        }
        return nullptr;
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
        auto* list = filesList(window);
        QVERIFY(list);
        QStringList files;
        for (int i = 0; i < list->count(); i++) {
            files.append(list->item(i)->text());
        }
        QVERIFY2(files.contains("com.example.target/main.lua"), qPrintable(files.join(", ")));
        QVERIFY(files.contains("prism.luaeditor/checks.lua"));
        QVERIFY(files.contains("com.example.target/plugin.toml"));

        list->setCurrentRow(static_cast<int>(files.indexOf("com.example.target/main.lua")));
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

        // Cannot switch files with unsaved changes
        const int current = list->currentRow();
        list->setCurrentRow(static_cast<int>(files.indexOf("prism.luaeditor/checks.lua")));
        QCOMPARE(list->currentRow(), current);
        QVERIFY(statusLabel(window)->text().contains("Save or revert"));

        // Saving writes the file
        editor->setPlainText("prism.log.info(\"saved\")\n");
        QTest::mouseClick(button(window, "Save"), Qt::LeftButton);
        QVERIFY2(statusLabel(window)->text().contains("Saved"), qPrintable(statusLabel(window)->text()));
        QFile saved(pluginsDir() + "/com.example.target/main.lua");
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), "prism.log.info(\"saved\")\n");
        QVERIFY(!window->windowTitle().endsWith("*"));

        // Revert goes back to the saved text
        editor->setPlainText("changed");
        QTest::mouseClick(button(window, "Revert"), Qt::LeftButton);
        QCOMPARE(editor->toPlainText(), "prism.log.info(\"saved\")\n");

        // Manifests open too, without Lua checks
        list->setCurrentRow(static_cast<int>(files.indexOf("com.example.target/plugin.toml")));
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
        const auto lists = window->findChildren<QListWidget*>();
        QListWidget* results = lists.first() == filesList(window) ? lists.last() : lists.first();
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
