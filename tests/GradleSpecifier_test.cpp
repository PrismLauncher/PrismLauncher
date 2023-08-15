#include <QTest>

#include <minecraft/GradleSpecifier.h>

class GradleSpecifierTest : public QObject {
    Q_OBJECT
   private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

    void test_Positive_data()
    {
        QTest::addColumn<QString>("through");

        QTest::newRow("3 parter") << "org.gradle.test.classifiers:service:1.0";
        QTest::newRow("classifier") << "org.gradle.test.classifiers:service:1.0:jdk15";
        QTest::newRow("jarextension") << "org.gradle.test.classifiers:service:1.0@jar";
        QTest::newRow("jarboth") << "org.gradle.test.classifiers:service:1.0:jdk15@jar";
        QTest::newRow("packxz") << "org.gradle.test.classifiers:service:1.0:jdk15@jar.pack.xz";
    }
    void test_Positive()
    {
        QFETCH(QString, through);

        QString converted = GradleSpecifier(through).serialize();

        QCOMPARE(converted, through);
    }

    void test_Path_data()
    {
        QTest::addColumn<QString>("spec");
        QTest::addColumn<QString>("expected");

        QTest::newRow("3 parter") << "group.id:artifact:1.0"
                                  << "group/id/artifact/1.0/artifact-1.0.jar";
        QTest::newRow("doom") << "id.software:doom:1.666:demons@wad"
                              << "id/software/doom/1.666/doom-1.666-demons.wad";
    }
    void test_Path()
    {
        QFETCH(QString, spec);
        QFETCH(QString, expected);

        QString converted = GradleSpecifier(spec).toPath();

        QCOMPARE(converted, expected);
    }
    void test_Negative_data()
    {
        QTest::addColumn<QString>("input");

        QTest::newRow("too many :") << "org:gradle.test:class:::ifiers:service:1.0::";
        QTest::newRow("nonsense") << "I like turtles";
        QTest::newRow("empty string") << "";
        QTest::newRow("missing version") << "herp.derp:artifact";
    }
    void test_Negative()
    {
        QFETCH(QString, input);

        GradleSpecifier spec(input);
        QVERIFY(!spec.valid());
        QCOMPARE(spec.serialize(), input);
        QCOMPARE(spec.toPath(), QString());
    }
};

QTEST_GUILESS_MAIN(GradleSpecifierTest)

#include "GradleSpecifier_test.moc"
