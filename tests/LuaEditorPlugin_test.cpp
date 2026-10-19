#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
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

    void test_editAndCheck()
    {
        const auto actions = m_manager->actions(PrismApi::ActionScope::Launcher);
        QCOMPARE(actions.size(), 1);
        QCOMPARE(actions.first().text, "Lua editor");
        QCOMPARE(m_manager->runAction(actions.first()), std::nullopt);

        QDialog* window = editorWindow();
        QVERIFY(window);
        auto* list = window->findChild<QListWidget*>();
        QStringList files;
        for (int i = 0; i < list->count(); i++) {
            files.append(list->item(i)->text());
        }
        QVERIFY2(files.contains("com.example.target/main.lua"), qPrintable(files.join(", ")));
        QVERIFY(files.contains("prism.luaeditor/checks.lua"));

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
        QVERIFY(window->findChild<QLabel*>()->text().contains("Save or revert"));

        // Saving writes the file
        editor->setPlainText("prism.log.info(\"saved\")\n");
        QTest::mouseClick(button(window, "Save"), Qt::LeftButton);
        QVERIFY2(window->findChild<QLabel*>()->text().contains("Saved"), qPrintable(window->findChild<QLabel*>()->text()));
        QFile saved(pluginsDir() + "/com.example.target/main.lua");
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), "prism.log.info(\"saved\")\n");
        QVERIFY(!window->windowTitle().endsWith("*"));

        // Revert goes back to the saved text
        editor->setPlainText("changed");
        QTest::mouseClick(button(window, "Revert"), Qt::LeftButton);
        QCOMPARE(editor->toPlainText(), "prism.log.info(\"saved\")\n");
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
