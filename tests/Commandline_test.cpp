#include <QTest>

#include <Commandline.h>

class CommandlineTest : public QObject {
    Q_OBJECT
   private slots:
    void test_splitArgs_data()
    {
        QTest::addColumn<QString>("args");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("plain arguments") << "a b c" << QStringList{ "a", "b", "c" };
        QTest::newRow("quoted argument with spaces") << "a \"b c\" d" << QStringList{ "a", "b c", "d" };
        QTest::newRow("windows path without quotes") << "-Dorg.lwjgl.glfw.libname=C:\\Users\\user\\glfw3.dll"
                                                     << QStringList{ "-Dorg.lwjgl.glfw.libname=C:\\Users\\user\\glfw3.dll" };
        QTest::newRow("windows path in quotes keeps backslashes")
            << "-Dorg.lwjgl.glfw.libname=\"C:\\Users\\test user\\glfw3.dll\""
            << QStringList{ "-Dorg.lwjgl.glfw.libname=C:\\Users\\test user\\glfw3.dll" };
        QTest::newRow("fully quoted argument keeps backslashes")
            << "\"-Dorg.lwjgl.glfw.libname=C:\\Users\\test user\\glfw3.dll\""
            << QStringList{ "-Dorg.lwjgl.glfw.libname=C:\\Users\\test user\\glfw3.dll" };
        QTest::newRow("windows path in single quotes keeps backslashes")
            << "-Dfoo='C:\\Users\\test user\\glfw3.dll'" << QStringList{ "-Dfoo=C:\\Users\\test user\\glfw3.dll" };
        QTest::newRow("escaped quotes") << "-Dfoo=\"say \\\"hi\\\" now\"" << QStringList{ "-Dfoo=say \"hi\" now" };
        QTest::newRow("double backslash in quotes collapses")
            << "-Dfoo=\"C:\\\\path\\\\file.dll\"" << QStringList{ "-Dfoo=C:\\path\\file.dll" };
    }
    void test_splitArgs()
    {
        QFETCH(QString, args);
        QFETCH(QStringList, expected);

        QCOMPARE(Commandline::splitArgs(args), expected);
    }
};

QTEST_GUILESS_MAIN(CommandlineTest)
#include "Commandline_test.moc"
