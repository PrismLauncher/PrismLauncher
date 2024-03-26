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
        auto dataDir = QDir(QFINDTESTDATA("testdata/CatPacks")).absolutePath();
        auto fileName = FS::PathCombine(dataDir, "index.json");
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
