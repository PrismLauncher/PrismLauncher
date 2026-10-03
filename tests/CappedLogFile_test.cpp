#include <QTemporaryDir>
#include <QTest>

#include <CappedLogFile.h>

class CappedLogFileTest : public QObject {
    Q_OBJECT
   private slots:

    void test_writesPassThroughBelowLimit()
    {
        QTemporaryDir dir;
        QString path = dir.filePath("log.txt");

        {
            CappedLogFile log(path, 1000);
            QVERIFY(log.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
            QByteArray first = "first line\n";
            QByteArray second = "second line\n";
            log.write(first);
            log.write(second);
            log.flush();

            QCOMPARE(log.bytesWritten(), first.size() + second.size());
        }

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray contents = file.readAll();
        contents.replace("\r\n", "\n");
        QCOMPARE(contents, QByteArray("first line\nsecond line\n"));
    }

    void test_writeStopsAtLimit()
    {
        QTemporaryDir dir;
        QString path = dir.filePath("log.txt");

        {
            CappedLogFile log(path, 100);
            QVERIFY(log.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
            for (int i = 0; i < 1000; i++) {
                log.write(QByteArray("this line is exactly forty characters long!\n"));
            }
            log.flush();
            log.close();

            // the cap fills the file up to 100 bytes, then appends the notice
            QCOMPARE(log.bytesWritten(), 100);
            QVERIFY(log.isLimitReached());
        }

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray contents = file.readAll();
        contents.replace("\r\n", "\n");
        QVERIFY(contents.size() < 250);
        QVERIFY(contents.count("this line is exactly forty characters long!") == 2);
        QVERIFY(contents.contains("maximum size of 100 bytes"));
    }

    void test_discardsWritesAfterLimit()
    {
        QTemporaryDir dir;
        QString path = dir.filePath("log.txt");

        CappedLogFile log(path, 10);
        QVERIFY(log.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
        log.write("0123456789");
        log.flush();
        log.write("this write would exceed the limit\n");
        log.write("and so would this one\n");
        log.flush();
        QCOMPARE(log.bytesWritten(), 10);

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray contents = file.readAll();
        contents.replace("\r\n", "\n");
        QCOMPARE(contents.count("maximum size of 10 bytes"), 1);
    }
};

QTEST_GUILESS_MAIN(CappedLogFileTest)
#include "CappedLogFile_test.moc"
