#include <QTest>

#include <QDate>
#include <QFileInfo>
#include <QList>
#include <QTemporaryFile>
#include "FileSystem.h"
#include "ui/themes/CatPack.h"

class CatPackTest : public QObject {
    Q_OBJECT
   private slots:
    void test_catPack()
    {
        QString fileContent = R"({
  "name": "My Cute Cat",
  "default": "maxwell.png",
  "variants": [
    {
      "startTime": { "day": 12, "month": 4 },
      "endTime": { "day": 12, "month": 4 },
      "path": "oneDay.png"
    },
    {
      "startTime": { "day": 20, "month": 12 },
      "endTime": { "day": 28, "month": 12 },
      "path": "christmas.png"
    },
     {
      "startTime": { "day": 30, "month": 12 },
      "endTime": { "day": 1, "month": 1 },
      "path": "newyear2.png"
    },
    {
      "startTime": { "day": 28, "month": 12 },
      "endTime": { "day": 3, "month": 1 },
      "path": "newyear.png"
    }
  ]
})";
#if defined(Q_OS_WIN)
        QString fileName = "test_SaveAlreadyExistingFile.ini";
        QFile file(fileName);
        QCOMPARE(file.open(QFile::WriteOnly | QFile::Text), true);
#else
        QTemporaryFile file;
        QCOMPARE(file.open(), true);
        QCOMPARE(file.fileName().isEmpty(), false);
        QString fileName = file.fileName();
#endif
        QTextStream stream(&file);
        stream << fileContent;
        file.close();
        auto fileinfo = QFileInfo(fileName);
        try {
            auto cat = JsonCatPack(fileinfo);
            QCOMPARE(cat.path(QDate(2023, 4, 12)), FS::PathCombine(fileinfo.path(), "oneDay.png"));
            QCOMPARE(cat.path(QDate(2023, 4, 11)), FS::PathCombine(fileinfo.path(), "maxwell.png"));
            QCOMPARE(cat.path(QDate(2023, 4, 13)), FS::PathCombine(fileinfo.path(), "maxwell.png"));
            QCOMPARE(cat.path(QDate(2023, 12, 21)), FS::PathCombine(fileinfo.path(), "christmas.png"));
            QCOMPARE(cat.path(QDate(2023, 12, 28)), FS::PathCombine(fileinfo.path(), "christmas.png"));
            QCOMPARE(cat.path(QDate(2023, 12, 29)), FS::PathCombine(fileinfo.path(), "newyear.png"));
            QCOMPARE(cat.path(QDate(2023, 12, 30)), FS::PathCombine(fileinfo.path(), "newyear2.png"));
            QCOMPARE(cat.path(QDate(2023, 12, 31)), FS::PathCombine(fileinfo.path(), "newyear2.png"));
            QCOMPARE(cat.path(QDate(2024, 1, 1)), FS::PathCombine(fileinfo.path(), "newyear2.png"));
            QCOMPARE(cat.path(QDate(2024, 1, 2)), FS::PathCombine(fileinfo.path(), "newyear.png"));
            QCOMPARE(cat.path(QDate(2024, 1, 3)), FS::PathCombine(fileinfo.path(), "newyear.png"));
            QCOMPARE(cat.path(QDate(2024, 1, 4)), FS::PathCombine(fileinfo.path(), "maxwell.png"));
        } catch (const Exception& e) {
            QFAIL(e.cause().toLatin1());
        }
    }
};

QTEST_GUILESS_MAIN(CatPackTest)

#include "CatPack_test.moc"
