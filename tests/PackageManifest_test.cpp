#include <QTest>
#include <QDebug>

#include <mojang/PackageManifest.h>

using namespace mojang_files;

QDebug operator<<(QDebug debug, const Path &path)
{
    debug << path.toString();
    return debug;
}

class PackageManifestTest : public QObject
{
    Q_OBJECT

private slots:
    void test_parse();
    void test_parse_file();
    void test_inspect();
#ifndef Q_OS_WIN32
    void test_inspect_symlinks();
#endif
    void mkdir_deep();
    void rmdir_deep();

    void identical_file();
    void changed_file();
    void added_file();
    void removed_file();
};

namespace {
QByteArray basic_manifest = R"END(
{
    "files": {
        "a/b.txt": {
            "type": "file",
            "downloads": {
                "raw": {
                    "url": "http://dethware.org/b.txt",
                    "sha1": "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                    "size": 0
                }
            },
            "executable": true
        },
        "a/b/c": {
            "type": "directory"
        },
        "a/b/c.txt": {
            "type": "link",
            "target": "../b.txt"
        }
    }
}
)END";
}

void PackageManifestTest::test_parse()
{
    auto manifest = Package::fromManifestContents(basic_manifest);
    QVERIFY(manifest.valid == true);
    QVERIFY(manifest.files.size() == 1);
    QVERIFY(manifest.files.count(Path("a/b.txt")));
    auto &file = manifest.files[Path("a/b.txt")];
    QVERIFY(file.executable == true);
    QVERIFY(file.hash == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    QVERIFY(file.size == 0);
    QVERIFY(manifest.folders.size() == 4);
    QVERIFY(manifest.folders.count(Path(".")));
    QVERIFY(manifest.folders.count(Path("a")));
    QVERIFY(manifest.folders.count(Path("a/b")));
    QVERIFY(manifest.folders.count(Path("a/b/c")));
    QVERIFY(manifest.symlinks.size() == 1);
    auto symlinkPath = Path("a/b/c.txt");
    QVERIFY(manifest.symlinks.count(symlinkPath));
    auto &symlink = manifest.symlinks[symlinkPath];
    QVERIFY(symlink == Path("../b.txt"));
    QVERIFY(manifest.sources.size() == 1);
}

void PackageManifestTest::test_parse_file() {
    auto path = QFINDTESTDATA("testdata/PackageManifest/1.8.0_202-x64.json");
    auto manifest = Package::fromManifestFile(path);
    QVERIFY(manifest.valid == true);
}


void PackageManifestTest::test_inspect() {
    auto path = QFINDTESTDATA("testdata/PackageManifest/inspect_win/");
    auto manifest = Package::fromInspectedFolder(path);
    QVERIFY(manifest.valid == true);
    QVERIFY(manifest.files.size() == 2);
    QVERIFY(manifest.files.count(Path("a/b.txt")));
    auto &file1 = manifest.files[Path("a/b.txt")];
    QVERIFY(file1.executable == false);
    QVERIFY(file1.hash == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    QVERIFY(file1.size == 0);
    QVERIFY(manifest.files.count(Path("a/b/b.txt")));
    auto &file2 = manifest.files[Path("a/b/b.txt")];
    QVERIFY(file2.executable == false);
    QVERIFY(file2.hash == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    QVERIFY(file2.size == 0);
    QVERIFY(manifest.folders.size() == 3);
    QVERIFY(manifest.folders.count(Path(".")));
    QVERIFY(manifest.folders.count(Path("a")));
    QVERIFY(manifest.folders.count(Path("a/b")));
    QVERIFY(manifest.symlinks.size() == 0);
}

#ifndef Q_OS_WIN32
void PackageManifestTest::test_inspect_symlinks() {
    auto path = QFINDTESTDATA("testdata/PackageManifest/inspect/");
    auto manifest = Package::fromInspectedFolder(path);
    QVERIFY(manifest.valid == true);
    QVERIFY(manifest.files.size() == 1);
    QVERIFY(manifest.files.count(Path("a/b.txt")));
    auto &file = manifest.files[Path("a/b.txt")];
    QVERIFY(file.executable == true);
    QVERIFY(file.hash == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    QVERIFY(file.size == 0);
    QVERIFY(manifest.folders.size() == 3);
    QVERIFY(manifest.folders.count(Path(".")));
    QVERIFY(manifest.folders.count(Path("a")));
    QVERIFY(manifest.folders.count(Path("a/b")));
    QVERIFY(manifest.symlinks.size() == 1);
    QVERIFY(manifest.symlinks.count(Path("a/b/b.txt")));
    qDebug() << manifest.symlinks[Path("a/b/b.txt")];
    QVERIFY(manifest.symlinks[Path("a/b/b.txt")] == Path("../b.txt"));
}
#endif

void PackageManifestTest::mkdir_deep() {

    Package from;
    auto to = Package::fromManifestContents(R"END(
{
    "files": {
        "a/b/c/d/e": {
            "type": "directory"
        }
    }
}
)END");
    auto operations = UpdateOperations::resolve(from, to);
    QVERIFY(operations.deletes.size() == 0);
    QVERIFY(operations.rmdirs.size() == 0);

    QVERIFY(operations.mkdirs.size() == 6);
    QVERIFY(operations.mkdirs[0] == Path("."));
    QVERIFY(operations.mkdirs[1] == Path("a"));
    QVERIFY(operations.mkdirs[2] == Path("a/b"));
    QVERIFY(operations.mkdirs[3] == Path("a/b/c"));
    QVERIFY(operations.mkdirs[4] == Path("a/b/c/d"));
    QVERIFY(operations.mkdirs[5] == Path("a/b/c/d/e"));

    QVERIFY(operations.downloads.size() == 0);
    QVERIFY(operations.mklinks.size() == 0);
    QVERIFY(operations.executable_fixes.size() == 0);
}

void PackageManifestTest::rmdir_deep() {

    Package to;
    auto from = Package::fromManifestContents(R"END(
{
    "files": {
        "a/b/c/d/e": {
            "type": "directory"
        }
    }
}
)END");
    auto operations = UpdateOperations::resolve(from, to);
    QVERIFY(operations.deletes.size() == 0);

    QVERIFY(operations.rmdirs.size() == 6);
    QVERIFY(operations.rmdirs[0] == Path("a/b/c/d/e"));
    QVERIFY(operations.rmdirs[1] == Path("a/b/c/d"));
    QVERIFY(operations.rmdirs[2] == Path("a/b/c"));
    QVERIFY(operations.rmdirs[3] == Path("a/b"));
    QVERIFY(operations.rmdirs[4] == Path("a"));
    QVERIFY(operations.rmdirs[5] == Path("."));

    QVERIFY(operations.mkdirs.size() == 0);
    QVERIFY(operations.downloads.size() == 0);
    QVERIFY(operations.mklinks.size() == 0);
    QVERIFY(operations.executable_fixes.size() == 0);
}

void PackageManifestTest::identical_file() {
    QByteArray manifest = R"END(
{
    "files": {
        "a/b/c/d/empty.txt": {
            "type": "file",
            "downloads": {
                "raw": {
                    "url": "http://dethware.org/empty.txt",
                    "sha1": "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                    "size": 0
                }
            },
            "executable": false
        }
    }
}
)END";
    auto from = Package::fromManifestContents(manifest);
    auto to = Package::fromManifestContents(manifest);
    auto operations = UpdateOperations::resolve(from, to);
    QVERIFY(operations.deletes.size() == 0);
    QVERIFY(operations.rmdirs.size() == 0);
    QVERIFY(operations.mkdirs.size() == 0);
    QVERIFY(operations.downloads.size() == 0);
    QVERIFY(operations.mklinks.size() == 0);
    QVERIFY(operations.executable_fixes.size() == 0);
}

void PackageManifestTest::changed_file() {
    auto from = Package::fromManifestContents(R"END(
{
    "files": {
        "a/b/c/d/file": {
            "type": "file",
            "downloads": {
                "raw": {
                    "url": "http://dethware.org/empty.txt",
                    "sha1": "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                    "size": 0
                }
            },
            "executable": false
        }
    }
}
)END");
    auto to = Package::fromManifestContents(R"END(
{
    "files": {
        "a/b/c/d/file": {
            "type": "file",
            "downloads": {
                "raw": {
                    "url": "http://dethware.org/space.txt",
                    "sha1": "dd122581c8cd44d0227f9c305581ffcb4b6f1b46",
                    "size": 1
                }
            },
            "executable": false
        }
    }
}
)END");
    auto operations = UpdateOperations::resolve(from, to);
    QVERIFY(operations.deletes.size() == 1);
    QCOMPARE(operations.deletes[0], Path("a/b/c/d/file"));
    QVERIFY(operations.rmdirs.size() == 0);
    QVERIFY(operations.mkdirs.size() == 0);
    QVERIFY(operations.downloads.size() == 1);
    QVERIFY(operations.mklinks.size() == 0);
    QVERIFY(operations.executable_fixes.size() == 0);
}

void PackageManifestTest::added_file() {
    auto from = Package::fromManifestContents(R"END(
{
    "files": {
        "a/b/c/d": {
            "type": "directory"
        }
    }
}
)END");
    auto to = Package::fromManifestContents(R"END(
{
    "files": {
        "a/b/c/d/file": {
            "type": "file",
            "downloads": {
                "raw": {
                    "url": "http://dethware.org/space.txt",
                    "sha1": "dd122581c8cd44d0227f9c305581ffcb4b6f1b46",
                    "size": 1
                }
            },
            "executable": false
        }
    }
}
)END");
    auto operations = UpdateOperations::resolve(from, to);
    QVERIFY(operations.deletes.size() == 0);
    QVERIFY(operations.rmdirs.size() == 0);
    QVERIFY(operations.mkdirs.size() == 0);
    QVERIFY(operations.downloads.size() == 1);
    QVERIFY(operations.mklinks.size() == 0);
    QVERIFY(operations.executable_fixes.size() == 0);
}

void PackageManifestTest::removed_file() {
    auto from = Package::fromManifestContents(R"END(
{
    "files": {
        "a/b/c/d/file": {
            "type": "file",
            "downloads": {
                "raw": {
                    "url": "http://dethware.org/space.txt",
                    "sha1": "dd122581c8cd44d0227f9c305581ffcb4b6f1b46",
                    "size": 1
                }
            },
            "executable": false
        }
    }
}
)END");
    auto to = Package::fromManifestContents(R"END(
{
    "files": {
        "a/b/c/d": {
            "type": "directory"
        }
    }
}
)END");
    auto operations = UpdateOperations::resolve(from, to);
    QVERIFY(operations.deletes.size() == 1);
    QCOMPARE(operations.deletes[0], Path("a/b/c/d/file"));
    QVERIFY(operations.rmdirs.size() == 0);
    QVERIFY(operations.mkdirs.size() == 0);
    QVERIFY(operations.downloads.size() == 0);
    QVERIFY(operations.mklinks.size() == 0);
    QVERIFY(operations.executable_fixes.size() == 0);
}

QTEST_GUILESS_MAIN(PackageManifestTest)

#include "PackageManifest_test.moc"

