#include <QApplication>
#include <QStyle>
#include <QtTest>

#include "ui/themes/SystemTheme.h"

class SystemThemeTest : public QObject {
    Q_OBJECT

   private slots:
    void reapplyUsesCurrentSystemPalette();
};

void SystemThemeTest::reapplyUsesCurrentSystemPalette()
{
    const auto originalPalette = QApplication::palette();
    const auto styleName = QApplication::style()->objectName();

    auto stalePalette = originalPalette;
    const QColor staleWindowColor(1, 2, 3);
    stalePalette.setColor(QPalette::Window, staleWindowColor);
    QApplication::setPalette(stalePalette);

    SystemTheme theme(styleName, stalePalette, true);
    theme.apply(false);

    QVERIFY(QApplication::palette().window().color() != staleWindowColor);
    QCOMPARE(theme.colorScheme(), QApplication::palette());

    QApplication::setPalette(originalPalette);
}

QTEST_MAIN(SystemThemeTest)

#include "SystemTheme_test.moc"
