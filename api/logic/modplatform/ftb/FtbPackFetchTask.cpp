#include "FtbPackFetchTask.h"
#include <QDomDocument>

FtbPackFetchTask::FtbPackFetchTask()
{
}

FtbPackFetchTask::~FtbPackFetchTask()
{
}

void FtbPackFetchTask::fetch()
{
    NetJob *netJob = new NetJob("FtbModpackFetch");

    QUrl publicPacksUrl = QUrl("https://ftb.cursecdn.com/FTB2/static/modpacks.xml");
    qDebug() << "Downloading public version info from" << publicPacksUrl.toString();
    netJob->addNetAction(Net::Download::makeByteArray(publicPacksUrl, &publicModpacksXmlFileData));

    QUrl thirdPartyUrl = QUrl("https://ftb.cursecdn.com/FTB2/static/thirdparty.xml");
    qDebug() << "Downloading thirdparty version info from" << thirdPartyUrl.toString();
    netJob->addNetAction(Net::Download::makeByteArray(thirdPartyUrl, &thirdPartyModpacksXmlFileData));

    QObject::connect(netJob, &NetJob::succeeded, this, &FtbPackFetchTask::fileDownloadFinished);
    QObject::connect(netJob, &NetJob::failed, this, &FtbPackFetchTask::fileDownloadFailed);

    jobPtr.reset(netJob);
    netJob->start();
}

void FtbPackFetchTask::fileDownloadFinished()
{
    jobPtr.reset();

    QStringList failedLists;

    if(!parseAndAddPacks(publicModpacksXmlFileData, FtbPackType::Public, publicPacks)) {
        failedLists.append(tr("Public Packs"));
    }

    if(!parseAndAddPacks(thirdPartyModpacksXmlFileData, FtbPackType::ThirdParty, thirdPartyPacks)) {
        failedLists.append(tr("Third Party Packs"));
    }

    if(failedLists.size() > 0) {
        emit failed(QString("Failed to download some pack lists:%1").arg(failedLists.join("\n- ")));
    } else {
        emit finished(publicPacks, thirdPartyPacks);
    }
}

bool FtbPackFetchTask::parseAndAddPacks(QByteArray &data, FtbPackType packType, FtbModpackList &list)
{
    QDomDocument doc;

    QString errorMsg = "Unknown error.";
    int errorLine = -1;
    int errorCol = -1;

    if(!doc.setContent(data, false, &errorMsg, &errorLine, &errorCol)){
        auto fullErrMsg = QString("Failed to fetch modpack data: %s %d:%d!").arg(errorMsg, errorLine, errorCol);
        qWarning() << fullErrMsg;
        data.clear();
        return false;
    }

    QDomNodeList nodes = doc.elementsByTagName("modpack");
    for(int i = 0; i < nodes.length(); i++) {
        QDomElement element = nodes.at(i).toElement();

        FtbModpack modpack;
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
        for(QString curr : modpack.oldVersions) {
            if(curr.isNull() || curr.isEmpty()) {
                modpack.oldVersions.removeAll(curr);
                modpack.bugged = true;
                qWarning() << "Removed some empty versions from" << modpack.name;
            }
        }

        if(modpack.oldVersions.size() < 1) {
            if(!modpack.currentVersion.isNull() && !modpack.currentVersion.isEmpty()) {
                modpack.oldVersions.append(modpack.currentVersion);
                qWarning() << "Added current version to oldVersions because oldVersions was empty! (" + modpack.name + ")";
            } else {
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

void FtbPackFetchTask::fileDownloadFailed(QString reason){
    qWarning() << "Fetching FtbPacks failed: " << reason;
    emit failed(reason);
}
