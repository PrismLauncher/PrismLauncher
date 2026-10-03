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
};

QTEST_GUILESS_MAIN(CappedLogFileTest)
#include "CappedLogFile_test.moc"
