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
    auto [request, response] = Net::Upload::makeByteArray(url, xbox_auth_data.toUtf8());
    m_request = request;
    m_request->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(headers));
    m_request->enableAutoRetry(true);

    m_task.reset(new NetJob("XboxUserStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });

    m_task->start();
    qDebug() << "First layer of Xbox auth ... commencing.";
}

void XboxUserStep::onRequestDone(QByteArray* response)
{
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        if (Net::isApplicationError(m_request->error())) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Xbox user authentication failed: %1").arg(m_request->errorString()));
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE, tr("Xbox user authentication failed: %1").arg(m_request->errorString()));
        }
        return;
    }

    Token temp;
    if (!Parsers::parseXTokenResponse(*response, temp, "UToken")) {
        qWarning() << "Could not parse user authentication response...";
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Xbox user authentication response could not be understood."));
        return;
    }
    m_data->userToken = temp;
    emit finished(AccountTaskState::STATE_WORKING, tr("Got Xbox user token"));
}
