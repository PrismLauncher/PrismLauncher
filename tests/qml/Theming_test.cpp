// SPDX-FileCopyrightText: 2023 flow <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <QTest>

#include <QQuickStyle>

#include "Theming_tests_common.h"

class ThemingBasicTest : public QObject {
    Q_OBJECT

   private slots:
    void test_systemTheme()
    {
        setupSystemTheme();

        QVERIFY(!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_CONF"));
        QVERIFY(ThemeManager::usingQmlSystemTheme());
    }

    void test_defaultFusionTheme()
    {
        auto t = setupDefaultFusionTheme();

        QVERIFY(!ThemeManager::usingQmlSystemTheme());
        QCOMPARE(QQuickStyle::name(), "Fusion");
    }

    void test_customFluentTheme()
    {
        auto theme_path = QFINDTESTDATA("testdata/Theming/Fluent");
        auto t = setupCustomTheme(theme_path);

        QVERIFY(!ThemeManager::usingQmlSystemTheme());
        QCOMPARE(qgetenv("QT_QUICK_CONTROLS_CONF"), t.m_conf_file_path);
    }
};

QTEST_APPLESS_MAIN(ThemingBasicTest)

#include "Theming_test.moc"
