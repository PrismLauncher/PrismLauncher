#include <QObject>
#include <QSettings>
#include <QTemporaryFile>
#include <QTest>

#include "config/GlobalConfig.h"

using namespace Qt::Literals;

namespace {
class ConfigTest : public QObject {
    Q_OBJECT
   private:
    static QString makeTempFileName()
    {
        // NOTE: helper needed as the QTemporaryFile will remain open even if it's close()d
        // (see https://doc.qt.io/qt-6/qtemporaryfile.html#details)
        QTemporaryFile tempFile;
        if (!tempFile.open()) {
            return {};
        }
        return tempFile.fileName();
    }

    template <typename T>
    static void testPassthrough(const QString& src)
    {
        const QDir dataDir(QFINDTESTDATA("testdata/Config"));
        const auto srcPath = dataDir.filePath(src);

        QSettings srcSettings(srcPath, QSettings::IniFormat);
        srcSettings.setFallbacksEnabled(false);
        QVERIFY(srcSettings.status() == QSettings::NoError);

        const auto conf = T::load(srcPath);
        QVERIFY(conf.has_value());

        const auto tempA = makeTempFileName();
        QVERIFY(!tempA.isNull());
        const auto tempB = makeTempFileName();
        QVERIFY(!tempB.isNull());

        auto tempRm = qScopeGuard([&tempA, &tempB] {
            QFile::remove(tempA);
            QFile::remove(tempB);
        });

        QVERIFY(conf->save(tempA));

        // HACK: if two QSettings objects are constructed with the same absolute path Qt reuses the same underlying data
        // this means that if setValue("MyKey", true) is set on one object, and then read from another object of the same path the type
        // remains as bool even though this information would be lost when reading from disk and MyKey would actually just be interpreted as
        // QString
        QVERIFY(QFile::copy(tempA, tempB));

        QSettings savedSettings(tempB, QSettings::IniFormat);
        savedSettings.setFallbacksEnabled(false);
        QVERIFY(savedSettings.status() == QSettings::NoError);

        for (const auto& key : srcSettings.allKeys()) {
            QVERIFY(srcSettings.contains(key));

            const auto& srcVal = srcSettings.value(key);
            const auto& savedVal = savedSettings.value(key);

            if (srcVal != savedVal || srcVal.metaType() != savedVal.metaType()) {
                const QString strValMsg = u"when saving %1 in %2: changed from %3 (%4) to %5 (%6)"_s.arg(
                    key, src, srcVal.toString(), srcVal.metaType().name(), savedVal.toString(), savedVal.metaType().name());
                QFAIL(qPrintable(strValMsg));
            }
        }
    }
   private slots:
    static void testGlobalConfigPassthrough() { testPassthrough<GlobalConfig>("global_config.ini"); }
};
}  // namespace

QTEST_GUILESS_MAIN(ConfigTest)

#include "Config_test.moc"
