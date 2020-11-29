#include "FTBPackInstallTask.h"

#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

namespace ModpacksCH {

PackInstallTask::PackInstallTask(Modpack pack, QString version)
{
    m_pack = pack;
    m_version_name = version;
}

bool PackInstallTask::abort()
{
    return true;
}

void PackInstallTask::executeTask()
{
    // Find pack version
    bool found = false;
    VersionInfo version;

    for(auto vInfo : m_pack.versions) {
        if (vInfo.name == m_version_name) {
            found = true;
            version = vInfo;
            continue;
        }
    }

    if(!found) {
        emitFailed("failed to find pack version " + m_version_name);
        return;
    }

    auto *netJob = new NetJob("ModpacksCH::VersionFetch");
    auto searchUrl = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/modpack/%1/%2")
            .arg(m_pack.id).arg(version.id);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &PackInstallTask::onDownloadSucceeded);
    QObject::connect(netJob, &NetJob::failed, this, &PackInstallTask::onDownloadFailed);
}

void PackInstallTask::onDownloadSucceeded()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from FTB at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto obj = doc.object();

    ModpacksCH::Version version;
    try
    {
        ModpacksCH::loadVersion(version, obj);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }
    m_version = version;

    install();
}

void PackInstallTask::onDownloadFailed(QString reason)
{
    jobPtr.reset();
    emitFailed(reason);
}

void PackInstallTask::install()
{
    setStatus(tr("Installing modpack"));

    auto instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    for(auto target : m_version.targets) {
        if(target.type == "game" && target.name == "minecraft") {
            components->setComponentVersion("net.minecraft", target.version, true);
            continue;
        }
    }

    for(auto target : m_version.targets) {
        if(target.type == "modloader" && target.name == "forge") {
            components->setComponentVersion("net.minecraftforge", target.version, true);
        }
    }
    components->saveNow();

    jobPtr.reset(new NetJob(tr("Mod download")));
    for(auto file : m_version.files) {
        if(file.serverOnly) continue;

        auto relpath = FS::PathCombine("minecraft", file.path, file.name);
        auto path = FS::PathCombine(m_stagingPath, relpath);

        qDebug() << "Will download" << file.url << "to" << path;
        auto dl = Net::Download::makeFile(file.url, path);
        jobPtr->addNetAction(dl);
    }

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        jobPtr.reset();
        emitSucceeded();
    });
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        jobPtr.reset();

        // FIXME: Temporarily ignore file download failures (matching FTB's installer),
        // while FTB's data is fucked.
        qWarning() << "Failed to download files for modpack: " + reason;
        emitSucceeded();
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        setProgress(current, total);
    });

    setStatus(tr("Downloading mods..."));
    jobPtr->start();

    instance.setName(m_instName);
    instance.setIconKey(m_instIcon);
    instanceSettings->resumeSave();
}

}
