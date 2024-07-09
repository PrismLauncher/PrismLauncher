#include "XboxProfileStep.h"

#include <QNetworkRequest>
#include <QUrlQuery>

#include "Application.h"
#include "Logging.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"

XboxProfileStep::XboxProfileStep(AccountData* data) : AuthStep(data) {}

QString XboxProfileStep::describe()
{
    return tr("Fetching Xbox profile.");
}

void XboxProfileStep::perform()
{
    QUrl url("https://profile.xboxlive.com/users/me/profile/settings");
    QUrlQuery q;
    q.addQueryItem("settings",
                   "GameDisplayName,AppDisplayName,AppDisplayPicRaw,GameDisplayPicRaw,"
                   "PublicGamerpic,ShowUserAsAvatar,Gamerscore,Gamertag,ModernGamertag,ModernGamertagSuffix,"
                   "UniqueModernGamertag,AccountTier,TenureLevel,XboxOneRep,"
                   "PreferredColor,Location,Bio,Watermarks,"
                   "RealName,RealNameOverride,IsQuarantined");
    url.setQuery(q);
    auto headers = QList<Net::HeaderPair>{
        { "Content-Type", "application/json" },
        { "Accept", "application/json" },
        { "x-xbl-contract-version", "3" },
        { "Authorization", QString("XBL3.0 x=%1;%2").arg(m_data->userToken.extra["uhs"].toString(), m_data->xboxApiToken.token).toUtf8() }
    };

    m_response.reset(new QByteArray());
    m_request = Net::Download::makeByteArray(url, m_response);
    m_request->addHeaderProxy(new Net::RawHeaderProxy(headers));

    m_task.reset(new NetJob("XboxProfileStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &XboxProfileStep::onRequestDone);

    m_task->start();
    qDebug() << "Getting Xbox profile...";
}

void XboxProfileStep::onRequestDone()
{
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        qCDebug(authCredentials()) << *m_response;
        if (Net::isApplicationError(m_request->error())) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Failed to retrieve the Xbox profile: %1").arg(m_request->errorString()));
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE, tr("Failed to retrieve the Xbox profile: %1").arg(m_request->errorString()));
        }
        return;
    }

    qCDebug(authCredentials()) << "XBox profile: " << *m_response;

    emit finished(AccountTaskState::STATE_WORKING, tr("Got Xbox profile"));
}
