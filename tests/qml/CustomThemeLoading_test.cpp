// SPDX-FileCopyrightText: 2023 flow <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <QTest>

#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include "Theming_tests_common.h"

class CustomThemeLoadingTest : public QObject {
    Q_OBJECT

   private slots:
    void test_customFluentTheme_onScreen()
    {
        auto qml_path = QFINDTESTDATA("testdata/Theming/test_theming.qml");

        auto theme_path = QFINDTESTDATA("testdata/Theming/Fluent");
        auto t = setupCustomTheme(theme_path);

        QQmlApplicationEngine engine;

        bool exposed = false;
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, this, [&](auto&& obj, auto&&) { exposed = (obj != nullptr); });

        engine.load(qml_path);

        if (!exposed)
            QSKIP("The test environment doesn't support opening graphical windows.");

        QCOMPARE(QQuickStyle::name(), "Material");
    }
};

QTEST_MAIN(CustomThemeLoadingTest)

#include "CustomThemeLoading_test.moc"
