#include <archive.h>
#include <archive_entry.h>

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <archive/ArchiveWriter.h>

#include <memory>

class ArchiveTest : public QObject {
    Q_OBJECT

    static int archivePermissions(const QString& archivePath)
    {
        std::unique_ptr<archive, decltype(&archive_read_free)> input(archive_read_new(), &archive_read_free);
        if (!input) {
            return 0;
        }
        archive_read_support_format_all(input.get());
        archive_read_support_filter_all(input.get());

        auto archivePathWide = archivePath.toStdWString();
        if (archive_read_open_filename_w(input.get(), archivePathWide.data(), 10240) != ARCHIVE_OK) {
            return 0;
        }

        archive_entry* entry = nullptr;
        if (archive_read_next_header(input.get(), &entry) != ARCHIVE_OK) {
            return 0;
        }
        return archive_entry_perm(entry);
    }

   private slots:
    void test_ExportedFileHasPermissions()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        auto sourcePath = tempDir.filePath("source.txt");
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write("contents"), qint64(8));
        source.close();

        auto archivePath = tempDir.filePath("export.zip");
        MMCZip::ArchiveWriter output(archivePath);
        QVERIFY(output.open());
        QVERIFY(output.addFile(sourcePath, QStringLiteral("source.txt")));
        QVERIFY(output.close());

        QVERIFY(archivePermissions(archivePath) != 0);
    }
};

QTEST_GUILESS_MAIN(ArchiveTest)

#include "Archive_test.moc"
