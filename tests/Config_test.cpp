#include <QObject>
#include <QSettings>
#include <QTemporaryFile>
#include <QTest>

#include "config/GlobalConfig.h"

namespace {
class ConfigTest : public QObject {
    Q_OBJECT
   private:
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

        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.close();
        tempFile.setAutoRemove(false);

        QVERIFY(conf->save(tempFile.fileName()));

        QSettings savedSettings(tempFile.fileName(), QSettings::IniFormat);
        savedSettings.setFallbacksEnabled(false);
        QVERIFY(savedSettings.status() == QSettings::NoError);

        for (const auto& key : srcSettings.allKeys()) {
            QVERIFY(srcSettings.contains(key));

            const auto& srcVal = srcSettings.value(key);
            const auto& savedVal = savedSettings.value(key);

            QCOMPARE(key + '=' + savedVal.toString(), key + '=' + srcVal.toString());
        }
    }
   private slots:
    void testGlobalConfigPassthrough() { testPassthrough<GlobalConfig>("global_config.ini"); }
};
}  // namespace

QTEST_GUILESS_MAIN(ConfigTest)

#include "Config_test.moc"
