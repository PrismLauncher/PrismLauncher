#include "PackFetchTask.h"
#include "PrivatePackManager.h"

#include <QDomDocument>
#include "BuildConfig.h"
#include "Application.h"

namespace LegacyFTB {

void PackFetchTask::fetch()
{
    publicPacks.clear();
    thirdPartyPacks.clear();

    jobPtr = new NetJob("LegacyFTB::ModpackFetch", m_network);

    QUrl publicPacksUrl = QUrl(BuildConfig.LEGACY_FTB_CDN_BASE_URL + "static/modpacks.xml");
    qDebug() << "Downloading public version info from" << publicPacksUrl.toString();
    jobPtr->addNetAction(Net::Download::makeByteArray(publicPacksUrl, &publicModpacksXmlFileData));

    QUrl thirdPartyUrl = QUrl(BuildConfig.LEGACY_FTB_CDN_BASE_URL + "static/thirdparty.xml");
    qDebug() << "Downloading thirdparty version info from" << thirdPartyUrl.toString();
    jobPtr->addNetAction(Net::Download::makeByteArray(thirdPartyUrl, &thirdPartyModpacksXmlFileData));

    QObject::connect(jobPtr.get(), &NetJob::succeeded, this, &PackFetchTask::fileDownloadFinished);
    QObject::connect(jobPtr.get(), &NetJob::failed, this, &PackFetchTask::fileDownloadFailed);

    jobPtr->start();
}

void PackFetchTask::fetchPrivate(const QStringList & toFetch)
{
    QString privatePackBaseUrl = BuildConfig.LEGACY_FTB_CDN_BASE_URL + "static/%1.xml";

    for (auto &packCode: toFetch)
    {
        QByteArray *data = new QByteArray();
        NetJob *job = new NetJob("Fetching private pack", m_network);
        job->addNetAction(Net::Download::makeByteArray(privatePackBaseUrl.arg(packCode), data));

        QObject::connect(job, &NetJob::succeeded, this, [this, job, data, packCode]
        {
            ModpackList packs;
            parseAndAddPacks(*data, PackType::Private, packs);
            foreach(Modpack currentPack, packs)
            {
                currentPack.packCode = packCode;
                emit privateFileDownloadFinished(currentPack);
            }

            job->deleteLater();

            data->clear();
            delete data;
        });

        QObject::connect(job, &NetJob::failed, this, [this, job, packCode, data](QString reason)
        {
            emit privateFileDownloadFailed(reason, packCode);
            job->deleteLater();

            data->clear();
            delete data;
        });

        job->start();
    }
}

void PackFetchTask::fileDownloadFinished()
{
    jobPtr.reset();

    QStringList failedLists;

    if(!parseAndAddPacks(publicModpacksXmlFileData, PackType::Public, publicPacks))
    {
        failedLists.append(tr("Public Packs"));
    }

    if(!parseAndAddPacks(thirdPartyModpacksXmlFileData, PackType::ThirdParty, thirdPartyPacks))
    {
        failedLists.append(tr("Third Party Packs"));
    }

    if(failedLists.size() > 0)
    {
        emit failed(tr("Failed to download some pack lists: %1").arg(failedLists.join("\n- ")));
    }
    else
    {
        emit finished(publicPacks, thirdPartyPacks);
    }
}

bool PackFetchTask::parseAndAddPacks(QByteArray &data, PackType packType, ModpackList &list)
{
    QDomDocument doc;

    QString errorMsg = "Unknown error.";
    int errorLine = -1;
    int errorCol = -1;

    if(!doc.setContent(data, false, &errorMsg, &errorLine, &errorCol))
    {
        auto fullErrMsg = QString("Failed to fetch modpack data: %1 %2:3d!").arg(errorMsg, errorLine, errorCol);
        qWarning() << fullErrMsg;
        data.clear();
        return false;
    }

    QDomNodeList nodes = doc.elementsByTagName("modpack");
    for(int i = 0; i < nodes.length(); i++)
    {
        QDomElement element = nodes.at(i).toElement();

        Modpack modpack;
        modpack.name = element.attribute("name");
        modpack.currentVersion = element.attribute("version");
        modpack.mcVersion = element.attribute("mcVersion");
        modpack.description = element.attribute("description");
        modpack.mods = element.attribute("mods");
        modpack.logo = element.attribute("logo");
        modpack.oldVersions = element.attribute("oldVersions").split(";");
        modpack.broken = false;
        modpack.bugged = false;

        //remove empty if the xml is bugged
        for(QString curr : modpack.oldVersions)
        {
            if(curr.isNull() || curr.isEmpty())
            {
                modpack.oldVersions.removeAll(curr);
                modpack.bugged = true;
                qWarning() << "Removed some empty versions from" << modpack.name;
            }
        }

        if(modpack.oldVersions.size() < 1)
        {
            if(!modpack.currentVersion.isNull() && !modpack.currentVersion.isEmpty())
            {
                modpack.oldVersions.append(modpack.currentVersion);
                qWarning() << "Added current version to oldVersions because oldVersions was empty! (" + modpack.name + ")";
            }
            else
            {
                modpack.broken = true;
                qWarning() << "Broken pack:" << modpack.name << " => No valid version!";
            }
        }

        modpack.author = element.attribute("author");

        modpack.dir = element.attribute("dir");
        modpack.file = element.attribute("url");

        modpack.type = packType;

        list.append(modpack);
    }

    return true;
}

void PackFetchTask::fileDownloadFailed(QString reason)
{
    qWarning() << "Fetching FTBPacks failed:" << reason;
    emit failed(reason);
}

}
