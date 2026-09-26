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

        QString fileName;

        // HACK: file is kept open and windows doesn't like that *grumble*
        {
            QTemporaryFile tempFile;
            QVERIFY(tempFile.open());
            fileName = tempFile.fileName();
        }

        auto tempFileRm = qScopeGuard([&fileName] { QFile::remove(fileName); });

        QVERIFY(conf->save(fileName));

        QSettings savedSettings(fileName, QSettings::IniFormat);
        savedSettings.setFallbacksEnabled(false);
        QVERIFY(savedSettings.status() == QSettings::NoError);

        for (const auto& key : srcSettings.allKeys()) {
            QVERIFY(srcSettings.contains(key));

            const auto& srcVal = srcSettings.value(key);
            const auto& savedVal = savedSettings.value(key);

            if (srcVal != savedVal || srcVal.metaType() != savedVal.metaType()) {
                const QString strValMsg = u"when saving %1 in %2: expected %3 (%4) but got %5 (%6)"_s.arg(
                    key, src, srcVal.toString(), srcVal.metaType().name(), savedVal.toString(), savedVal.metaType().name());
                QFAIL(qPrintable(strValMsg));
            }
        }
    }
   private slots:
    void testGlobalConfigPassthrough() { testPassthrough<GlobalConfig>("global_config.ini"); }
};
}  // namespace

QTEST_GUILESS_MAIN(ConfigTest)

#include "Config_test.moc"
