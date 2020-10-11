#include <QTest>
#include <QDebug>
#include "TestUtil.h"

#include "mojang/PackageManifest.h"

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
    void test_diff();

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
    QCOMPARE(manifest.valid, true);
    QCOMPARE(manifest.files.size(), 1);
    QCOMPARE(manifest.files.count(Path("a/b.txt")), 1);
    auto &file = manifest.files[Path("a/b.txt")];
    QCOMPARE(file.executable, true);
    QCOMPARE(file.hash, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    QCOMPARE(file.size, 0);
    QCOMPARE(manifest.folders.size(), 4);
    QCOMPARE(manifest.folders.count(Path(".")), 1);
    QCOMPARE(manifest.folders.count(Path("a")), 1);
    QCOMPARE(manifest.folders.count(Path("a/b")), 1);
    QCOMPARE(manifest.folders.count(Path("a/b/c")), 1);
    QCOMPARE(manifest.symlinks.size(), 1);
    auto symlinkPath = Path("a/b/c.txt");
    QCOMPARE(manifest.symlinks.count(symlinkPath), 1);
    auto &symlink = manifest.symlinks[symlinkPath];
    QCOMPARE(symlink, Path("../b.txt"));
    QCOMPARE(manifest.sources.size(), 1);
}

void PackageManifestTest::test_parse_file() {
    auto path = QFINDTESTDATA("testdata/1.8.0_202-x64.json");
    auto manifest = Package::fromManifestFile(path);
    QCOMPARE(manifest.valid, true);
}

void PackageManifestTest::test_inspect() {
    auto path = QFINDTESTDATA("testdata/inspect/");
    auto manifest = Package::fromInspectedFolder(path);
    QCOMPARE(manifest.valid, true);
    QCOMPARE(manifest.files.size(), 1);
    QCOMPARE(manifest.files.count(Path("a/b.txt")), 1);
    auto &file = manifest.files[Path("a/b.txt")];
    QCOMPARE(file.executable, true);
    QCOMPARE(file.hash, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    QCOMPARE(file.size, 0);
    QCOMPARE(manifest.folders.size(), 3);
    QCOMPARE(manifest.folders.count(Path(".")), 1);
    QCOMPARE(manifest.folders.count(Path("a")), 1);
    QCOMPARE(manifest.folders.count(Path("a/b")), 1);
    QCOMPARE(manifest.symlinks.size(), 1);
}

void PackageManifestTest::test_diff() {
    auto path = QFINDTESTDATA("testdata/inspect/");
    auto from = Package::fromInspectedFolder(path);
    auto to = Package::fromManifestContents(basic_manifest);
    auto operations = UpdateOperations::resolve(from, to);
    QCOMPARE(operations.valid, true);
    QCOMPARE(operations.mkdirs.size(), 1);
    QCOMPARE(operations.mkdirs[0], Path("a/b/c"));

    QCOMPARE(operations.rmdirs.size(), 0);
    QCOMPARE(operations.deletes.size(), 1);
    QCOMPARE(operations.deletes[0], Path("a/b/b.txt"));
    QCOMPARE(operations.downloads.size(), 0);
    QCOMPARE(operations.mklinks.size(), 1);
    QCOMPARE(operations.mklinks.count(Path("a/b/c.txt")), 1);
    QCOMPARE(operations.mklinks[Path("a/b/c.txt")], Path("../b.txt"));
}

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
    QCOMPARE(operations.deletes.size(), 0);
    QCOMPARE(operations.rmdirs.size(), 0);

    QCOMPARE(operations.mkdirs.size(), 6);
    QCOMPARE(operations.mkdirs[0], Path("."));
    QCOMPARE(operations.mkdirs[1], Path("a"));
    QCOMPARE(operations.mkdirs[2], Path("a/b"));
    QCOMPARE(operations.mkdirs[3], Path("a/b/c"));
    QCOMPARE(operations.mkdirs[4], Path("a/b/c/d"));
    QCOMPARE(operations.mkdirs[5], Path("a/b/c/d/e"));

    QCOMPARE(operations.downloads.size(), 0);
    QCOMPARE(operations.mklinks.size(), 0);
    QCOMPARE(operations.executable_fixes.size(), 0);
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
    QCOMPARE(operations.deletes.size(), 0);

    QCOMPARE(operations.rmdirs.size(), 6);
    QCOMPARE(operations.rmdirs[0], Path("a/b/c/d/e"));
    QCOMPARE(operations.rmdirs[1], Path("a/b/c/d"));
    QCOMPARE(operations.rmdirs[2], Path("a/b/c"));
    QCOMPARE(operations.rmdirs[3], Path("a/b"));
    QCOMPARE(operations.rmdirs[4], Path("a"));
    QCOMPARE(operations.rmdirs[5], Path("."));

    QCOMPARE(operations.mkdirs.size(), 0);
    QCOMPARE(operations.downloads.size(), 0);
    QCOMPARE(operations.mklinks.size(), 0);
    QCOMPARE(operations.executable_fixes.size(), 0);
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
    QCOMPARE(operations.deletes.size(), 0);
    QCOMPARE(operations.rmdirs.size(), 0);
    QCOMPARE(operations.mkdirs.size(), 0);
    QCOMPARE(operations.downloads.size(), 0);
    QCOMPARE(operations.mklinks.size(), 0);
    QCOMPARE(operations.executable_fixes.size(), 0);
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
    QCOMPARE(operations.deletes.size(), 1);
    QCOMPARE(operations.deletes[0], Path("a/b/c/d/file"));
    QCOMPARE(operations.rmdirs.size(), 0);
    QCOMPARE(operations.mkdirs.size(), 0);
    QCOMPARE(operations.downloads.size(), 1);
    QCOMPARE(operations.mklinks.size(), 0);
    QCOMPARE(operations.executable_fixes.size(), 0);
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
    QCOMPARE(operations.deletes.size(), 0);
    QCOMPARE(operations.rmdirs.size(), 0);
    QCOMPARE(operations.mkdirs.size(), 0);
    QCOMPARE(operations.downloads.size(), 1);
    QCOMPARE(operations.mklinks.size(), 0);
    QCOMPARE(operations.executable_fixes.size(), 0);
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
    QCOMPARE(operations.deletes.size(), 1);
    QCOMPARE(operations.deletes[0], Path("a/b/c/d/file"));
    QCOMPARE(operations.rmdirs.size(), 0);
    QCOMPARE(operations.mkdirs.size(), 0);
    QCOMPARE(operations.downloads.size(), 0);
    QCOMPARE(operations.mklinks.size(), 0);
    QCOMPARE(operations.executable_fixes.size(), 0);
}

QTEST_GUILESS_MAIN(PackageManifestTest)

#include "PackageManifest_test.moc"

