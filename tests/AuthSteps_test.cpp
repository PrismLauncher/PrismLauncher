#include "AuthSteps_test.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTest>
#include <QTimer>

#include <algorithm>
#include <cstring>
#include <optional>

#include "BuildConfig.h"
#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AuthContext.h"
#include "minecraft/auth/AuthFlow.h"
#include "minecraft/auth/steps/MSADeviceCodeStep.h"
#include "minecraft/auth/steps/XboxUserStep.h"
#include "services/LauncherSettings.h"
#include "services/NetworkContext.h"

namespace {

/** Finished reply carrying a fixed HTTP status and body. */
class CannedReply : public QNetworkReply {
   public:
    CannedReply(QNetworkAccessManager::Operation op, const QNetworkRequest& request, int status, QByteArray body)
        : m_body(std::move(body))
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(op);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        setHeader(QNetworkRequest::ContentLengthHeader, m_body.size());
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (status >= 400) {
            setError(QNetworkReply::ProtocolInvalidOperationError, "canned HTTP error");
        }
        QTimer::singleShot(0, this, [this] {
            if (error() != QNetworkReply::NoError) {
                emit errorOccurred(error());
            }
            emit readyRead();
            setFinished(true);
            emit finished();
        });
    }

    void abort() override {}
    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return m_body.size() - m_offset + QIODevice::bytesAvailable(); }

   protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        auto count = std::min<qint64>(maxSize, m_body.size() - m_offset);
        std::memcpy(data, m_body.constData() + m_offset, count);
        m_offset += count;
        return count;
    }

   private:
    QByteArray m_body;
    qint64 m_offset = 0;
};

struct SentRequest {
    QUrl url;
    QByteArray body;
};

/** Answers requests from a URL -> body table and records what was sent. */
class CannedNetworkAccessManager : public QNetworkAccessManager {
   public:
    QHash<QUrl, QByteArray> responses;
    QList<SentRequest> sent;

   protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* outgoingData) override
    {
        sent.append({ request.url(), outgoingData != nullptr ? outgoingData->readAll() : QByteArray() });
        auto it = responses.constFind(request.url());
        if (it == responses.constEnd()) {
            return new CannedReply(op, request, 404, {});
        }
        return new CannedReply(op, request, 200, *it);
    }
};

class FakeNetworkContext : public NetworkContext {
   public:
    QNetworkAccessManager* qnam() override { return &nam; }
    HttpMetaCache* httpMetaCache() override { return nullptr; }
    QString userAgent() const override { return "test-agent"; }
    QString flameApiKey() const override { return {}; }
    QString modrinthApiToken() const override { return {}; }

    CannedNetworkAccessManager nam;
};

class FakeLauncherSettings : public LauncherSettings {
   public:
    QVariant get(const QString& id) override { return values.value(id); }
    bool set(const QString& id, QVariant value) override
    {
        values[id] = std::move(value);
        return true;
    }
    void reset(const QString& id) override { values.remove(id); }
    bool contains(const QString& id) override { return values.contains(id); }
    std::shared_ptr<Setting> getSetting(const QString&) const override { return nullptr; }
    std::shared_ptr<Setting> getOrRegisterSetting(const QString&, QVariant) override { return nullptr; }
    QVariant getPlaytime(const QString&) override { return {}; }
    bool setPlaytime(const QString&, QVariant) override { return false; }
    SettingsObject* settingsObject() override { return nullptr; }
    SettingsObject* playtimeSettingsObject() override { return nullptr; }

    QVariantMap values;
};

QByteArray cannedJson(const QJsonObject& object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

}  // namespace

void AuthStepsTest::test_msaClientIdFromInjectedSettings()
{
    FakeLauncherSettings settings;
    AuthContext context{ .settings = &settings };
    QCOMPARE(context.msaClientId(), BuildConfig.MSA_CLIENT_ID);

    settings.values["MSAClientIDOverride"] = "override-client-id";
    QCOMPARE(context.msaClientId(), QString("override-client-id"));

    AuthContext noSettings;
    QCOMPARE(noSettings.msaClientId(), BuildConfig.MSA_CLIENT_ID);
}

void AuthStepsTest::test_offlineFlowSucceedsWithoutServices()
{
    AccountData data;
    data.type = AccountType::Offline;

    AuthFlow flow(&data, AuthContext{}, AuthFlow::Action::Login);
    flow.start();

    QVERIFY(flow.wasSuccessful());
    QCOMPARE(flow.taskState(), AccountTaskState::STATE_SUCCEEDED);
    QCOMPARE(data.accountState, AccountState::Online);
}

void AuthStepsTest::test_refreshWithChangedClientIdIsDisabled()
{
    FakeNetworkContext network;
    FakeLauncherSettings settings;
    settings.values["MSAClientIDOverride"] = "new-client-id";

    AccountData data;
    data.type = AccountType::MSA;
    data.msaClientID = "old-client-id";
    data.msaToken.refresh_token = "refresh-token";

    AuthFlow flow(&data, AuthContext{ .network = &network, .settings = &settings }, AuthFlow::Action::Refresh);
    flow.start();

    QVERIFY(!flow.wasSuccessful());
    QCOMPARE(flow.taskState(), AccountTaskState::STATE_DISABLED);
    QCOMPARE(data.accountState, AccountState::Disabled);
    QVERIFY(network.nam.sent.isEmpty());
}

void AuthStepsTest::test_xboxUserStepUsesInjectedNetwork()
{
    FakeNetworkContext network;
    network.nam.responses[QUrl("https://user.auth.xboxlive.com/user/authenticate")] = cannedJson({
        { "IssueInstant", "2026-11-23T08:00:00.0000000Z" },
        { "NotAfter", "2026-12-07T08:00:00.0000000Z" },
        { "Token", "xbox-user-token" },
        { "DisplayClaims", QJsonObject{ { "xui", QJsonArray{ QJsonObject{ { "uhs", "user-hash" } } } } } },
    });

    AccountData data;
    data.type = AccountType::MSA;
    data.msaToken.token = "msa-access-token";

    XboxUserStep step(&data, &network);
    std::optional<AccountTaskState> result;
    connect(&step, &AuthStep::finished, this, [&result](AccountTaskState state, QString) { result = state; });
    step.perform();

    QTRY_VERIFY(result.has_value());
    QCOMPARE(*result, AccountTaskState::STATE_WORKING);
    QCOMPARE(data.userToken.token, QString("xbox-user-token"));
    QCOMPARE(data.userToken.extra.value("uhs").toString(), QString("user-hash"));

    QCOMPARE(network.nam.sent.size(), 1);
    QVERIFY(network.nam.sent.first().body.contains("d=msa-access-token"));
}

void AuthStepsTest::test_deviceCodeStepSendsInjectedClientId()
{
    FakeNetworkContext network;
    network.nam.responses[QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode")] = cannedJson({
        { "device_code", "device-code" },
        { "user_code", "USER-CODE" },
        { "verification_uri", "https://microsoft.com/link" },
        { "expires_in", 900 },
        { "interval", 5 },
    });

    AccountData data;
    data.type = AccountType::MSA;

    MSADeviceCodeStep step(&data, &network, "injected-client-id");
    QString userCode;
    connect(&step, &MSADeviceCodeStep::authorizeWithBrowser, this, [&userCode](QString, QString code, int) { userCode = code; });
    step.perform();

    QTRY_COMPARE(userCode, QString("USER-CODE"));
    step.abort();

    QCOMPARE(network.nam.sent.size(), 1);
    QVERIFY(network.nam.sent.first().body.contains("client_id=injected-client-id"));
}

QTEST_GUILESS_MAIN(AuthStepsTest)
