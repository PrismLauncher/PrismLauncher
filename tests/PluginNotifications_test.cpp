#include <QApplication>
#include <QFrame>
#include <QMainWindow>
#include <QTest>

#include <ui/widgets/PluginNotifications.h>

class PluginNotificationsTest : public QObject {
    Q_OBJECT

    static QList<QFrame*> popups(QWidget& window)
    {
        QList<QFrame*> result;
        for (auto* frame : window.findChildren<QFrame*>(Qt::FindDirectChildrenOnly)) {
            if (frame->isVisible() && frame->frameShape() == QFrame::StyledPanel) {
                result.append(frame);
            }
        }
        return result;
    }

   private slots:
    void test_stackAtBottomRight()
    {
        QWidget window;
        window.resize(800, 600);
        window.show();
        PluginNotifications notifications(&window);

        for (int i = 0; i < PluginNotifications::s_maxVisible + 2; i++) {
            notifications.show("Plugin", QString("message %1").arg(i), PrismApi::NotifyLevel::Info);
        }
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        const auto visible = popups(window);
        QCOMPARE(visible.size(), PluginNotifications::s_maxVisible);
        for (QFrame* popup : visible) {
            QVERIFY(window.rect().contains(popup->geometry()));
            QCOMPARE(popup->geometry().right(), window.width() - 12 - 1);
        }

        // Follows the window when it is resized
        window.resize(1000, 700);
        QCoreApplication::processEvents();
        for (QFrame* popup : popups(window)) {
            QCOMPARE(popup->geometry().right(), 1000 - 12 - 1);
            QVERIFY(popup->geometry().bottom() < 700);
        }
    }
};

int main(int argc, char* argv[])
{
    // No real screen needed
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    PluginNotificationsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "PluginNotifications_test.moc"
