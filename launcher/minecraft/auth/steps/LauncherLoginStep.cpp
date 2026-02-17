#include "LauncherLoginStep.h"

#include <QNetworkRequest>
#include <QUrl>

#include "Application.h"
#include "Logging.h"
#include "minecraft/auth/Parsers.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"
#include "net/Upload.h"

LauncherLoginStep::LauncherLoginStep(AccountData* data) : AuthStep(data) {}

QString LauncherLoginStep::describe()
{
    return tr("Fetching Minecraft access token");
}

void LauncherLoginStep::perform()
{
    QUrl url("https://api.minecraftservices.com/launcher/login");
    auto uhs = m_data->mojangservicesToken.extra["uhs"].toString();
    auto xToken = m_data->mojangservicesToken.token;

    QString mc_auth_template = R"XXX(
{
    "xtoken": "XBL3.0 x=%1;%2",
    "platform": "PC_LAUNCHER"
}
)XXX";
    auto requestBody = mc_auth_template.arg(uhs, xToken);

    auto headers = QList<Net::HeaderPair>{
        { "Content-Type", "application/json" },
        { "Accept", "application/json" },
    };

    auto [request, response] = Net::Upload::makeByteArray(url, requestBody.toUtf8());
    m_request = request;
    m_request->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(headers));
    m_request->enableAutoRetry(true);

    m_task.reset(new NetJob("LauncherLoginStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });

    m_task->start();
    qDebug() << "Getting Minecraft access token...";
}

void LauncherLoginStep::onRequestDone(QByteArray* response)
{
    qCDebug(authCredentials()) << *response;
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        if (Net::isApplicationError(m_request->error())) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT,
                          tr("Failed to get Minecraft access token: %1").arg(m_request->errorString()));
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE, tr("Failed to get Minecraft access token: %1").arg(m_request->errorString()));
        }
        return;
    }

    if (!Parsers::parseMojangResponse(*response, m_data->yggdrasilToken)) {
        qWarning() << "Could not parse login_with_xbox response...";
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Failed to parse the Minecraft access token response."));
        return;
    }
    emit finished(AccountTaskState::STATE_WORKING, tr("Got Minecraft access token"));
}
