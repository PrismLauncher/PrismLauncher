#include <QNetworkAccessManager>
#include <QTest>

#include "BuildConfig.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "net/ApiHeaderProxy.h"
#include "net/NetJob.h"
#include "services/NetworkContext.h"

namespace {
class FakeNetworkContext : public NetworkContext {
   public:
    QNetworkAccessManager* qnam() override { return &m_nam; }
    HttpMetaCache* httpMetaCache() override { return nullptr; }
    QString userAgent() const override { return "test-agent"; }
    QString flameApiKey() const override { return flameKey; }
    QString modrinthApiToken() const override { return modrinthToken; }

    QString flameKey;
    QString modrinthToken;

   private:
    QNetworkAccessManager m_nam;
};

QByteArray headerValue(const QList<Net::HeaderPair>& headers, const QByteArray& name)
{
    for (const auto& header : headers) {
        if (header.headerName == name) {
            return header.headerValue;
        }
    }
    return {};
}
}  // namespace

class ModPlatformInjectionTest : public QObject {
    Q_OBJECT

   private slots:
    void test_flameKeyFromInjectedContext()
    {
        FakeNetworkContext network;
        network.flameKey = "injected-flame-key";
        Net::ApiHeaderProxy proxy(&network);

        QNetworkRequest api(QUrl(BuildConfig.FLAME_BASE_URL + "/mods/1"));
        QCOMPARE(headerValue(proxy.headers(api), "x-api-key"), QByteArray("injected-flame-key"));

        QNetworkRequest cdn(QUrl("https://" + BuildConfig.FLAME_DOWNLOAD_HOST + "/files/1/2/a.jar"));
        QCOMPARE(headerValue(proxy.headers(cdn), "x-api-key"), QByteArray("injected-flame-key"));
    }

    void test_emptyFlameKeySendsNoHeader()
    {
        FakeNetworkContext network;
        Net::ApiHeaderProxy proxy(&network);

        QNetworkRequest api(QUrl(BuildConfig.FLAME_BASE_URL + "/mods/1"));
        QVERIFY(proxy.headers(api).isEmpty());
    }

    void test_modrinthTokenFromInjectedContext()
    {
        FakeNetworkContext network;
        network.modrinthToken = "mrp_token";
        Net::ApiHeaderProxy proxy(&network);

        QNetworkRequest api(QUrl(BuildConfig.MODRINTH_PROD_URL + "/project/abc"));
        QCOMPARE(headerValue(proxy.headers(api), "Authorization"), QByteArray("mrp_token"));
    }

    void test_credentialsNotLeakedToOtherHosts()
    {
        FakeNetworkContext network;
        network.flameKey = "injected-flame-key";
        network.modrinthToken = "mrp_token";
        Net::ApiHeaderProxy proxy(&network);

        QNetworkRequest other(QUrl("https://example.com/file"));
        QVERIFY(proxy.headers(other).isEmpty());
    }

    void test_nullContextOnlyAddsDownloadMeta()
    {
        Net::ApiHeaderProxy proxy(nullptr, Net::ModrinthDownloadMeta{ .reason = "update" });

        QNetworkRequest api(QUrl(BuildConfig.FLAME_BASE_URL + "/mods/1"));
        QVERIFY(proxy.headers(api).isEmpty());

        QNetworkRequest cdn(QUrl("https://" + BuildConfig.MODRINTH_DOWNLOAD_HOST + "/data/a.jar"));
        QVERIFY(!headerValue(proxy.headers(cdn), "modrinth-download-meta").isEmpty());
    }

    void test_apisBuildJobsOnInjectedNetwork()
    {
        FakeNetworkContext network;
        FlameAPI flame(&network);
        ModrinthAPI modrinth(&network);
        QVERIFY(flame.network() == &network);
        QVERIFY(modrinth.network() == &network);

        auto [flameTask, flameResponse] = flame.getFiles({ "1", "2" });
        QVERIFY(flameTask != nullptr);
        QVERIFY(flameResponse != nullptr);
        QVERIFY(qSharedPointerDynamicCast<NetJob>(flameTask) != nullptr);

        auto [modrinthTask, modrinthResponse] = modrinth.currentVersions({ "abc" }, "sha1");
        QVERIFY(modrinthTask != nullptr);
        QVERIFY(modrinthResponse != nullptr);
    }
};

QTEST_GUILESS_MAIN(ModPlatformInjectionTest)

#include "ModPlatformInjection_test.moc"
