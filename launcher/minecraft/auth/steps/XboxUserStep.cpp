#include "XboxUserStep.h"

#include <QNetworkRequest>

#include "Application.h"
#include "minecraft/auth/Parsers.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"

XboxUserStep::XboxUserStep(AccountData* data) : AuthStep(data) {}

QString XboxUserStep::describe()
{
    return tr("Logging in as an Xbox user.");
}

void XboxUserStep::perform()
{
    QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "AuthMethod": "RPS",
        "SiteName": "user.auth.xboxlive.com",
        "RpsTicket": "d=%1"
    },
    "RelyingParty": "http://auth.xboxlive.com",
    "TokenType": "JWT"
}
)XXX";
    auto xbox_auth_data = xbox_auth_template.arg(m_data->msaToken.token);

    QUrl url("https://user.auth.xboxlive.com/user/authenticate");
    auto headers = QList<Net::HeaderPair>{
        { "Content-Type", "application/json" },
        { "Accept", "application/json" },
        // set contract-version header (prevent err 400 bad-request?)
        // https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/reference/live/rest/additional/httpstandardheaders
        { "x-xbl-contract-version", "1" }
    };
    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, xbox_auth_data.toUtf8());
    m_request->addHeaderProxy(new Net::RawHeaderProxy(headers));

    m_task.reset(new NetJob("XboxUserStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &XboxUserStep::onRequestDone);

    m_task->start();
    qDebug() << "First layer of XBox auth ... commencing.";
}

void XboxUserStep::onRequestDone()
{
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        if (Net::isApplicationError(m_request->error())) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("XBox user authentication failed: %1").arg(m_request->errorString()));
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE, tr("XBox user authentication failed: %1").arg(m_request->errorString()));
        }
        return;
    }

    Token temp;
    if (!Parsers::parseXTokenResponse(*m_response, temp, "UToken")) {
        qWarning() << "Could not parse user authentication response...";
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("XBox user authentication response could not be understood."));
        return;
    }
    m_data->userToken = temp;
    emit finished(AccountTaskState::STATE_WORKING, tr("Got Xbox user token"));
}
