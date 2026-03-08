#include "ElyByStep.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUuid>

#include "Application.h"
#include "net/NetJob.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"
#include "net/Upload.h"

ElyByStep::ElyByStep(AccountData* data, bool refresh) : AuthStep(data), m_refresh(refresh) {}

QString ElyByStep::describe()
{
    return m_refresh ? tr("Refreshing Ely.by session") : tr("Logging in with Ely.by account");
}

void ElyByStep::perform()
{
    static const QUrl kAuthServerBaseUrl("https://authserver.ely.by/api/authlib-injector/authserver/");

    const auto clientToken = m_data->yggdrasilToken.extra.value("clientToken").toString().isEmpty()
                                 ? QUuid::createUuid().toString(QUuid::Id128)
                                 : m_data->yggdrasilToken.extra.value("clientToken").toString();

    QJsonObject payload;
    payload.insert("clientToken", clientToken);
    payload.insert("requestUser", true);

    QUrl url;
    if (m_refresh && !m_data->yggdrasilToken.token.isEmpty()) {
        url = kAuthServerBaseUrl.resolved(QUrl("refresh"));
        payload.insert("accessToken", m_data->yggdrasilToken.token);
    } else {
        const auto login = m_data->yggdrasilToken.extra.value("elyLogin").toString();
        const auto password = m_data->yggdrasilToken.extra.value("elyPassword").toString();
        if (login.isEmpty() || password.isEmpty()) {
            emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Ely.by credentials are missing."));
            return;
        }

        url = kAuthServerBaseUrl.resolved(QUrl("authenticate"));
        payload.insert("username", login);
        payload.insert("password", password);
        payload.insert("agent", QJsonObject{ { "name", "Minecraft" }, { "version", 1 } });
    }

    auto headers = QList<Net::HeaderPair>{
        { "Content-Type", "application/json" },
        { "Accept", "application/json" },
    };

    auto [request, response] = Net::Upload::makeByteArray(url, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_request = request;
    m_request->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(headers));
    m_request->enableAutoRetry(true);

    m_task.reset(new NetJob("ElyByStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);
    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });
    m_task->start();
}

void ElyByStep::onRequestDone(QByteArray* response)
{
    if (m_request->error() != QNetworkReply::NoError) {
        if (Net::isApplicationError(m_request->error())) {
            emit finished(AccountTaskState::STATE_FAILED_HARD,
                          tr("Failed to authenticate with Ely.by: %1").arg(m_request->errorString()));
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE,
                          tr("Failed to authenticate with Ely.by: %1").arg(m_request->errorString()));
        }
        return;
    }

    QJsonParseError jsonError;
    auto doc = QJsonDocument::fromJson(*response, &jsonError);
    if (jsonError.error || !doc.isObject()) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Failed to parse Ely.by authentication response."));
        return;
    }

    auto obj = doc.object();
    const auto accessToken = obj.value("accessToken").toString();
    const auto clientToken = obj.value("clientToken").toString();
    auto selectedProfile = obj.value("selectedProfile").toObject();

    const auto profileId = selectedProfile.value("id").toString();
    const auto profileName = selectedProfile.value("name").toString();

    if (accessToken.isEmpty() || profileId.isEmpty() || profileName.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Ely.by did not return a valid Minecraft profile."));
        return;
    }

    m_data->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();
    m_data->yggdrasilToken.notAfter = m_data->yggdrasilToken.issueInstant.addSecs(24 * 3600);
    m_data->yggdrasilToken.token = accessToken;
    m_data->yggdrasilToken.validity = Validity::Certain;
    if (!clientToken.isEmpty()) {
        m_data->yggdrasilToken.extra["clientToken"] = clientToken;
    }

    m_data->minecraftProfile.id = profileId;
    m_data->minecraftProfile.name = profileName;
    m_data->minecraftProfile.validity = Validity::Certain;

    m_data->minecraftEntitlement.ownsMinecraft = true;
    m_data->minecraftEntitlement.canPlayMinecraft = true;
    m_data->minecraftEntitlement.validity = Validity::Certain;

    emit finished(AccountTaskState::STATE_WORKING, tr("Authenticated with Ely.by"));
}
