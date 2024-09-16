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
    return tr("Accessing Mojang services.");
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

    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, requestBody.toUtf8());
    m_request->addHeaderProxy(new Net::RawHeaderProxy(headers));

    m_task.reset(new NetJob("LauncherLoginStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &LauncherLoginStep::onRequestDone);

    m_task->start();
    qDebug() << "Getting Minecraft access token...";
}

void LauncherLoginStep::onRequestDone()
{
    qCDebug(authCredentials()) << *m_response;
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

    if (!Parsers::parseMojangResponse(*m_response, m_data->yggdrasilToken)) {
        qWarning() << "Could not parse login_with_xbox response...";
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Failed to parse the Minecraft access token response."));
        return;
    }
    emit finished(AccountTaskState::STATE_WORKING, tr(""));
}
