#include "LauncherLoginStep.h"

#include <QNetworkRequest>
#include <QUrl>

#include "Application.h"
#include "Logging.h"
#include "minecraft/auth/Parsers.h"
#include "net/NetUtils.h"
#include "net/StaticHeaderProxy.h"
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
    m_task = Net::Upload::makeByteArray(url, m_response, requestBody.toUtf8());
    m_task->addHeaderProxy(new Net::StaticHeaderProxy(headers));

    connect(m_task.get(), &Task::finished, this, &LauncherLoginStep::onRequestDone);

    m_task->setNetwork(APPLICATION->network());
    m_task->start();
    qDebug() << "Getting Minecraft access token...";
}

void LauncherLoginStep::onRequestDone()
{
    qCDebug(authCredentials()) << *m_response;
    if (m_task->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_task->error();
        if (Net::isApplicationError(m_task->error())) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Failed to get Minecraft access token: %1").arg(m_task->errorString()));
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE, tr("Failed to get Minecraft access token: %1").arg(m_task->errorString()));
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
