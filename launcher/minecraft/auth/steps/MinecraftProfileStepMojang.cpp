#include "MinecraftProfileStepMojang.h"

#include <QNetworkRequest>

#include "Logging.h"
#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"
#include "net/NetUtils.h"

MinecraftProfileStepMojang::MinecraftProfileStepMojang(AccountData* data) : AuthStep(data) {}

MinecraftProfileStepMojang::~MinecraftProfileStepMojang() noexcept = default;

QString MinecraftProfileStepMojang::describe()
{
    return tr("Fetching the Minecraft profile.");
}

void MinecraftProfileStepMojang::perform()
{
    if (m_data->minecraftProfile.id.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, tr("A UUID is required to get the profile."));
        return;
    }

    // use session server instead of profile due to profile endpoint being locked for locked Mojang accounts
    QUrl url = QUrl("https://sessionserver.mojang.com/session/minecraft/profile/" + m_data->minecraftProfile.id);
    QNetworkRequest req = QNetworkRequest(url);
    AuthRequest* request = new AuthRequest(this);
    connect(request, &AuthRequest::finished, this, &MinecraftProfileStepMojang::onRequestDone);
    request->get(req);
}

void MinecraftProfileStepMojang::rehydrate()
{
    // NOOP, for now. We only save bools and there's nothing to check.
}

void MinecraftProfileStepMojang::onRequestDone(QNetworkReply::NetworkError error,
                                               QByteArray data,
                                               QList<QNetworkReply::RawHeaderPair> headers)
{
    auto requestor = qobject_cast<AuthRequest*>(QObject::sender());
    requestor->deleteLater();

    qCDebug(authCredentials()) << data;
    if (error == QNetworkReply::ContentNotFoundError) {
        // NOTE: Succeed even if we do not have a profile. This is a valid account state.
        if (m_data->type == AccountType::Mojang) {
            m_data->minecraftEntitlement.canPlayMinecraft = false;
            m_data->minecraftEntitlement.ownsMinecraft = false;
        }
        m_data->minecraftProfile = MinecraftProfile();
        emit finished(AccountTaskState::STATE_SUCCEEDED, tr("Account has no Minecraft profile."));
        return;
    }
    if (error != QNetworkReply::NoError) {
        qWarning() << "Error getting profile:";
        qWarning() << " HTTP Status:        " << requestor->httpStatus_;
        qWarning() << " Internal error no.: " << error;
        qWarning() << " Error string:       " << requestor->errorString_;

        qWarning() << " Response:";
        qWarning() << QString::fromUtf8(data);

        if (Net::isApplicationError(error)) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT,
                          tr("Minecraft Java profile acquisition failed: %1").arg(requestor->errorString_));
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE,
                          tr("Minecraft Java profile acquisition failed: %1").arg(requestor->errorString_));
        }
        return;
    }
    if (!Parsers::parseMinecraftProfileMojang(data, m_data->minecraftProfile)) {
        m_data->minecraftProfile = MinecraftProfile();
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Minecraft Java profile response could not be parsed"));
        return;
    }

    if (m_data->type == AccountType::Mojang) {
        auto validProfile = m_data->minecraftProfile.validity == Katabasis::Validity::Certain;
        m_data->minecraftEntitlement.canPlayMinecraft = validProfile;
        m_data->minecraftEntitlement.ownsMinecraft = validProfile;
    }
    emit finished(AccountTaskState::STATE_WORKING, tr("Minecraft Java profile acquisition succeeded."));
}
