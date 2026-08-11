#include <AppKit/AppKit.h>

#include <QApplication>
#include <QWidget>
#include <QtTest>

#include "ui/themes/ThemeManager.h"

namespace {
QColor backgroundColor(NSWindow* window) {
    NSColor* color = [window.backgroundColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    return QColor::fromRgbF(color.redComponent, color.greenComponent, color.blueComponent, color.alphaComponent);
}
}  // namespace

class MacOSThemeManagerTest : public QObject {
    Q_OBJECT

   private slots:
    void observerUsesCurrentApplicationPalette();
};

void MacOSThemeManagerTest::observerUsesCurrentApplicationPalette() {
    ThemeManager manager;
    QWidget widget;
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    manager.setApplicationTheme("system", true);

    auto palette = QApplication::palette();
    const QColor currentWindowColor(12, 34, 56);
    palette.setColor(QPalette::Window, currentWindowColor);
    QApplication::setPalette(palette);

    NSView* view = reinterpret_cast<NSView*>(widget.winId());
    NSWindow* window = view.window;
    [[NSNotificationCenter defaultCenter] postNotificationName:NSWindowDidChangeOcclusionStateNotification object:window];

    QCOMPARE(backgroundColor(window), currentWindowColor);
}

QTEST_MAIN(MacOSThemeManagerTest)

#include "MacOSThemeManager_test.moc"
