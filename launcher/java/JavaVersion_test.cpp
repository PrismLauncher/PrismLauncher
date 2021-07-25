#include <QTest>
#include "TestUtil.h"

#include "java/JavaVersion.h"

class JavaVersionTest : public QObject
{
    Q_OBJECT
private
slots:
    void test_Parse_data()
    {
        QTest::addColumn<QString>("string");
        QTest::addColumn<int>("major");
        QTest::addColumn<int>("minor");
        QTest::addColumn<int>("security");
        QTest::addColumn<QString>("prerelease");

        QTest::newRow("old format") << "1.6.0_33" << 6 << 0 << 33 << QString();
        QTest::newRow("old format prerelease") << "1.9.0_1-ea" << 9 << 0 << 1 << "ea";

        QTest::newRow("new format major") << "9" << 9 << 0 << 0 << QString();
        QTest::newRow("new format minor") << "9.1" << 9 << 1 << 0 << QString();
        QTest::newRow("new format security") << "9.0.1" << 9 << 0 << 1 << QString();
        QTest::newRow("new format prerelease") << "9-ea" << 9 << 0 << 0 << "ea";
        QTest::newRow("new format long prerelease") << "9.0.1-ea" << 9 << 0 << 1 << "ea";
    }
    void test_Parse()
    {
        QFETCH(QString, string);
        QFETCH(int, major);
        QFETCH(int, minor);
        QFETCH(int, security);
        QFETCH(QString, prerelease);

        JavaVersion test(string);
        QCOMPARE(test.m_string, string);
        QCOMPARE(test.toString(), string);
        QCOMPARE(test.m_major, major);
        QCOMPARE(test.m_minor, minor);
        QCOMPARE(test.m_security, security);
        QCOMPARE(test.m_prerelease, prerelease);
    }

    void test_Sort_data()
    {
        QTest::addColumn<QString>("lhs");
        QTest::addColumn<QString>("rhs");
        QTest::addColumn<bool>("smaller");
        QTest::addColumn<bool>("equal");
        QTest::addColumn<bool>("bigger");

        // old format and new format equivalence
        QTest::newRow("1.6.0_33 == 6.0.33") << "1.6.0_33" << "6.0.33" << false << true << false;
        // old format major version
        QTest::newRow("1.5.0_33 < 1.6.0_33") << "1.5.0_33" << "1.6.0_33" << true << false << false;
        // new format - first release vs first security patch
        QTest::newRow("9 < 9.0.1") << "9" << "9.0.1" << true << false << false;
        QTest::newRow("9.0.1 > 9") << "9.0.1" << "9" << false << false << true;
        // new format - first minor vs first release/security patch
        QTest::newRow("9.1 > 9.0.1") << "9.1" << "9.0.1" << false << false << true;
        QTest::newRow("9.0.1 < 9.1") << "9.0.1" << "9.1" << true << false << false;
        QTest::newRow("9.1 > 9") << "9.1" << "9" << false << false << true;
        QTest::newRow("9 > 9.1") << "9" << "9.1" << true << false << false;
        // new format - omitted numbers
        QTest::newRow("9 == 9.0") << "9" << "9.0" << false << true << false;
        QTest::newRow("9 == 9.0.0") << "9" << "9.0.0" << false << true << false;
        QTest::newRow("9.0 == 9.0.0") << "9.0" << "9.0.0" << false << true << false;
        // early access and prereleases compared to final release
        QTest::newRow("9-ea < 9") << "9-ea" << "9" << true << false << false;
        QTest::newRow("9 < 9.0.1-ea") << "9" << "9.0.1-ea" << true << false << false;
        QTest::newRow("9.0.1-ea > 9") << "9.0.1-ea" << "9" << false << false << true;
        // prerelease difference only testing
        QTest::newRow("9-1 == 9-1") << "9-1" << "9-1" << false << true << false;
        QTest::newRow("9-1 < 9-2") << "9-1" << "9-2" << true << false << false;
        QTest::newRow("9-5 < 9-20") << "9-5" << "9-20" << true << false << false;
        QTest::newRow("9-rc1 < 9-rc2") << "9-rc1" << "9-rc2" << true << false << false;
        QTest::newRow("9-rc5 < 9-rc20") << "9-rc5" << "9-rc20" << true << false << false;
        QTest::newRow("9-rc < 9-rc2") << "9-rc" << "9-rc2" << true << false << false;
        QTest::newRow("9-ea < 9-rc") << "9-ea" << "9-rc" << true << false << false;
    }
    void test_Sort()
    {
        QFETCH(QString, lhs);
        QFETCH(QString, rhs);
        QFETCH(bool, smaller);
        QFETCH(bool, equal);
        QFETCH(bool, bigger);
        JavaVersion lver(lhs);
        JavaVersion rver(rhs);
        QCOMPARE(lver < rver, smaller);
        QCOMPARE(lver == rver, equal);
        QCOMPARE(lver > rver, bigger);
    }
    void test_PermGen_data()
    {
        QTest::addColumn<QString>("version");
        QTest::addColumn<bool>("needs_permgen");
        QTest::newRow("1.6.0_33") << "1.6.0_33" << true;
        QTest::newRow("1.7.0_60") << "1.7.0_60" << true;
        QTest::newRow("1.8.0_22") << "1.8.0_22" << false;
        QTest::newRow("9-ea") << "9-ea" << false;
        QTest::newRow("9.2.4") << "9.2.4" << false;
    }
    void test_PermGen()
    {
        QFETCH(QString, version);
        QFETCH(bool, needs_permgen);
        JavaVersion v(version);
        QCOMPARE(needs_permgen, v.requiresPermGen());
    }
};

QTEST_GUILESS_MAIN(JavaVersionTest)

#include "JavaVersion_test.moc"
