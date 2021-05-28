#include <Env.h>
#include <quazip.h>
#include <QtConcurrent/QtConcurrent>
#include <MMCZip.h>
#include <minecraft/OneSixVersionFormat.h>
#include <Version.h>
#include "ATLPackInstallTask.h"

#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"

namespace ATLauncher {

PackInstallTask::PackInstallTask(UserInteractionSupport *support, QString pack, QString version)
{
    m_support = support;
    m_pack = pack;
    m_version_name = version;
}

bool PackInstallTask::abort()
{
    return true;
}

void PackInstallTask::executeTask()
{
    qDebug() << "PackInstallTask::executeTask: " << QThread::currentThreadId();
    auto *netJob = new NetJob("ATLauncher::VersionFetch");
    auto searchUrl = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "packs/%1/versions/%2/Configs.json")
            .arg(m_pack).arg(m_version_name);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &PackInstallTask::onDownloadSucceeded);
    QObject::connect(netJob, &NetJob::failed, this, &PackInstallTask::onDownloadFailed);
}

void PackInstallTask::onDownloadSucceeded()
{
    qDebug() << "PackInstallTask::onDownloadSucceeded: " << QThread::currentThreadId();
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from FTB at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto obj = doc.object();

    ATLauncher::PackVersion version;
    try
    {
        ATLauncher::loadVersion(version, obj);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }
    m_version = version;

    auto vlist = ENV.metadataIndex()->get("net.minecraft");
    if(!vlist)
    {
        emitFailed(tr("Failed to get local metadata index for %1").arg("net.minecraft"));
        return;
    }

    auto ver = vlist->getVersion(m_version.minecraft);
    if (!ver) {
        emitFailed(tr("Failed to get local metadata index for '%1' v%2").arg("net.minecraft").arg(m_version.minecraft));
        return;
    }
    ver->load(Net::Mode::Online);
    minecraftVersion = ver;

    if(m_version.noConfigs) {
        downloadMods();
    }
    else {
        installConfigs();
    }
}

void PackInstallTask::onDownloadFailed(QString reason)
{
    qDebug() << "PackInstallTask::onDownloadFailed: " << QThread::currentThreadId();
    jobPtr.reset();
    emitFailed(reason);
}

QString PackInstallTask::getDirForModType(ModType type, QString raw)
{
    switch (type) {
        // Mod types that can either be ignored at this stage, or ignored
        // completely.
        case ModType::Root:
        case ModType::Extract:
        case ModType::Decomp:
        case ModType::TexturePackExtract:
        case ModType::ResourcePackExtract:
        case ModType::MCPC:
            return Q_NULLPTR;
        case ModType::Forge:
            // Forge detection happens later on, if it cannot be detected it will
            // install a jarmod component.
        case ModType::Jar:
            return "jarmods";
        case ModType::Mods:
            return "mods";
        case ModType::Flan:
            return "Flan";
        case ModType::Dependency:
            return FS::PathCombine("mods", m_version.minecraft);
        case ModType::Ic2Lib:
            return FS::PathCombine("mods", "ic2");
        case ModType::DenLib:
            return FS::PathCombine("mods", "denlib");
        case ModType::Coremods:
            return "coremods";
        case ModType::Plugins:
            return "plugins";
        case ModType::TexturePack:
            return "texturepacks";
        case ModType::ResourcePack:
            return "resourcepacks";
        case ModType::ShaderPack:
            return "shaderpacks";
        case ModType::Millenaire:
            qWarning() << "Unsupported mod type: " + raw;
            return Q_NULLPTR;
        case ModType::Unknown:
            emitFailed(tr("Unknown mod type: %1").arg(raw));
            return Q_NULLPTR;
    }

    return Q_NULLPTR;
}

QString PackInstallTask::getVersionForLoader(QString uid)
{
    if(m_version.loader.recommended || m_version.loader.latest || m_version.loader.choose) {
        auto vlist = ENV.metadataIndex()->get(uid);
        if(!vlist)
        {
            emitFailed(tr("Failed to get local metadata index for %1").arg(uid));
            return Q_NULLPTR;
        }

        if(!vlist->isLoaded()) {
            vlist->load(Net::Mode::Online);
        }

        if(m_version.loader.recommended || m_version.loader.latest) {
            for (int i = 0; i < vlist->versions().size(); i++) {
                auto version = vlist->versions().at(i);
                auto reqs = version->requires();

                // filter by minecraft version, if the loader depends on a certain version.
                // not all mod loaders depend on a given Minecraft version, so we won't do this
                // filtering for those loaders.
                if (m_version.loader.type != "fabric") {
                    auto iter = std::find_if(reqs.begin(), reqs.end(), [](const Meta::Require &req) {
                        return req.uid == "net.minecraft";
                    });
                    if (iter == reqs.end()) continue;
                    if (iter->equalsVersion != m_version.minecraft) continue;
                }

                if (m_version.loader.recommended) {
                    // first recommended build we find, we use.
                    if (!version->isRecommended()) continue;
                }

                return version->descriptor();
            }

            emitFailed(tr("Failed to find version for %1 loader").arg(m_version.loader.type));
            return Q_NULLPTR;
        }
        else if(m_version.loader.choose) {
            // Fabric Loader doesn't depend on a given Minecraft version.
            if (m_version.loader.type == "fabric") {
                return m_support->chooseVersion(vlist, Q_NULLPTR);
            }

            return m_support->chooseVersion(vlist, m_version.minecraft);
        }
    }

    if (m_version.loader.version == Q_NULLPTR || m_version.loader.version.isEmpty()) {
        emitFailed(tr("No loader version set for modpack!"));
        return Q_NULLPTR;
    }

    return m_version.loader.version;
}

QString PackInstallTask::detectLibrary(VersionLibrary library)
{
    // Try to detect what the library is
    if (!library.server.isEmpty() && library.server.split("/").length() >= 3) {
        auto lastSlash = library.server.lastIndexOf("/");
        auto locationAndVersion = library.server.mid(0, lastSlash);
        auto fileName = library.server.mid(lastSlash + 1);

        lastSlash = locationAndVersion.lastIndexOf("/");
        auto location = locationAndVersion.mid(0, lastSlash);
        auto version = locationAndVersion.mid(lastSlash + 1);

        lastSlash = location.lastIndexOf("/");
        auto group = location.mid(0, lastSlash).replace("/", ".");
        auto artefact = location.mid(lastSlash + 1);

        return group + ":" + artefact + ":" + version;
    }

    if(library.file.contains("-")) {
        auto lastSlash = library.file.lastIndexOf("-");
        auto name = library.file.mid(0, lastSlash);
        auto version = library.file.mid(lastSlash + 1).remove(".jar");

        if(name == QString("guava")) {
            return "com.google.guava:guava:" + version;
        }
        else if(name == QString("commons-lang3")) {
            return "org.apache.commons:commons-lang3:" + version;
        }
    }

    return "org.multimc.atlauncher:" + library.md5 + ":1";
}

bool PackInstallTask::createLibrariesComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile)
{
    if(m_version.libraries.isEmpty()) {
        return true;
    }

    QList<GradleSpecifier> exempt;
    for(const auto & componentUid : componentsToInstall.keys()) {
        auto componentVersion = componentsToInstall.value(componentUid);

        for(const auto & library : componentVersion->data()->libraries) {
            GradleSpecifier lib(library->rawName());
            exempt.append(lib);
        }
    }

    {
        for(const auto & library : minecraftVersion->data()->libraries) {
            GradleSpecifier lib(library->rawName());
            exempt.append(lib);
        }
    }

    auto uuid = QUuid::createUuid();
    auto id = uuid.toString().remove('{').remove('}');
    auto target_id = "org.multimc.atlauncher." + id;

    auto patchDir = FS::PathCombine(instanceRoot, "patches");
    if(!FS::ensureFolderPathExists(patchDir))
    {
        return false;
    }
    auto patchFileName = FS::PathCombine(patchDir, target_id + ".json");

    auto f = std::make_shared<VersionFile>();
    f->name = m_pack + " " + m_version_name + " (libraries)";

    for(const auto & lib : m_version.libraries) {
        auto libName = detectLibrary(lib);
        GradleSpecifier libSpecifier(libName);

        bool libExempt = false;
        for(const auto & existingLib : exempt) {
            if(libSpecifier.matchName(existingLib)) {
                // If the pack specifies a newer version of the lib, use that!
                libExempt = Version(libSpecifier.version()) >= Version(existingLib.version());
            }
        }
        if(libExempt) continue;

        auto library = std::make_shared<Library>();
        library->setRawName(libName);

        switch(lib.download) {
            case DownloadType::Server:
                library->setAbsoluteUrl(BuildConfig.ATL_DOWNLOAD_SERVER_URL + lib.url);
                break;
            case DownloadType::Direct:
                library->setAbsoluteUrl(lib.url);
                break;
            case DownloadType::Browser:
            case DownloadType::Unknown:
                emitFailed(tr("Unknown or unsupported download type: %1").arg(lib.download_raw));
                return false;
        }

        f->libraries.append(library);
    }

    if(f->libraries.isEmpty()) {
        return true;
    }

    QFile file(patchFileName);
    if (!file.open(QFile::WriteOnly))
    {
        qCritical() << "Error opening" << file.fileName()
                    << "for reading:" << file.errorString();
        return false;
    }
    file.write(OneSixVersionFormat::versionFileToJson(f).toJson());
    file.close();

    profile->appendComponent(new Component(profile.get(), target_id, f));
    return true;
}

bool PackInstallTask::createPackComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile)
{
    if(m_version.mainClass == QString() && m_version.extraArguments == QString()) {
        return true;
    }

    auto uuid = QUuid::createUuid();
    auto id = uuid.toString().remove('{').remove('}');
    auto target_id = "org.multimc.atlauncher." + id;

    auto patchDir = FS::PathCombine(instanceRoot, "patches");
    if(!FS::ensureFolderPathExists(patchDir))
    {
        return false;
    }
    auto patchFileName = FS::PathCombine(patchDir, target_id + ".json");

    QStringList mainClasses;
    QStringList tweakers;
    for(const auto & componentUid : componentsToInstall.keys()) {
        auto componentVersion = componentsToInstall.value(componentUid);

        if(componentVersion->data()->mainClass != QString("")) {
            mainClasses.append(componentVersion->data()->mainClass);
        }
        tweakers.append(componentVersion->data()->addTweakers);
    }

    auto f = std::make_shared<VersionFile>();
    f->name = m_pack + " " + m_version_name;
    if(m_version.mainClass != QString() && !mainClasses.contains(m_version.mainClass)) {
        f->mainClass = m_version.mainClass;
    }

    // Parse out tweakers
    auto args = m_version.extraArguments.split(" ");
    QString previous;
    for(auto arg : args) {
        if(arg.startsWith("--tweakClass=") || previous == "--tweakClass") {
            auto tweakClass = arg.remove("--tweakClass=");
            if(tweakers.contains(tweakClass)) continue;

            f->addTweakers.append(tweakClass);
        }
        previous = arg;
    }

    if(f->mainClass == QString() && f->addTweakers.isEmpty()) {
        return true;
    }

    QFile file(patchFileName);
    if (!file.open(QFile::WriteOnly))
    {
        qCritical() << "Error opening" << file.fileName()
                    << "for reading:" << file.errorString();
        return false;
    }
    file.write(OneSixVersionFormat::versionFileToJson(f).toJson());
    file.close();

    profile->appendComponent(new Component(profile.get(), target_id, f));
    return true;
}

void PackInstallTask::installConfigs()
{
    qDebug() << "PackInstallTask::installConfigs: " << QThread::currentThreadId();
    setStatus(tr("Downloading configs..."));
    jobPtr.reset(new NetJob(tr("Config download")));

    auto path = QString("Configs/%1/%2.zip").arg(m_pack).arg(m_version_name);
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "packs/%1/versions/%2/Configs.zip")
            .arg(m_pack).arg(m_version_name);
    auto entry = ENV.metacache()->resolveEntry("ATLauncherPacks", path);
    entry->setStale(true);

    jobPtr->addNetAction(Net::Download::makeCached(url, entry));
    archivePath = entry->getFullPath();

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        jobPtr.reset();
        extractConfigs();
    });
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        jobPtr.reset();
        emitFailed(reason);
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        setProgress(current, total);
    });

    jobPtr->start();
}

void PackInstallTask::extractConfigs()
{
    qDebug() << "PackInstallTask::extractConfigs: " << QThread::currentThreadId();
    setStatus(tr("Extracting configs..."));

    QDir extractDir(m_stagingPath);

    QuaZip packZip(archivePath);
    if(!packZip.open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Failed to open pack configs %1!").arg(archivePath));
        return;
    }

    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractDir, archivePath, extractDir.absolutePath() + "/minecraft");
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, [&]()
    {
        downloadMods();
    });
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, [&]()
    {
        emitAborted();
    });
    m_extractFutureWatcher.setFuture(m_extractFuture);
}

void PackInstallTask::downloadMods()
{
    qDebug() << "PackInstallTask::installMods: " << QThread::currentThreadId();

    QVector<ATLauncher::VersionMod> optionalMods;
    for (const auto& mod : m_version.mods) {
        if (mod.optional) {
            optionalMods.push_back(mod);
        }
    }

    // Select optional mods, if pack contains any
    QVector<QString> selectedMods;
    if (!optionalMods.isEmpty()) {
        setStatus(tr("Selecting optional mods..."));
        selectedMods = m_support->chooseOptionalMods(optionalMods);
    }

    setStatus(tr("Downloading mods..."));

    jarmods.clear();
    jobPtr.reset(new NetJob(tr("Mod download")));
    for(const auto& mod : m_version.mods) {
        // skip non-client mods
        if(!mod.client) continue;

        // skip optional mods that were not selected
        if(mod.optional && !selectedMods.contains(mod.name)) continue;

        QString url;
        switch(mod.download) {
            case DownloadType::Server:
                url = BuildConfig.ATL_DOWNLOAD_SERVER_URL + mod.url;
                break;
            case DownloadType::Browser:
                emitFailed(tr("Unsupported download type: %1").arg(mod.download_raw));
                return;
            case DownloadType::Direct:
                url = mod.url;
                break;
            case DownloadType::Unknown:
                emitFailed(tr("Unknown download type: %1").arg(mod.download_raw));
                return;
        }

        QFileInfo fileName(mod.file);
        auto cacheName = fileName.completeBaseName() + "-" + mod.md5 + "." + fileName.suffix();

        if (mod.type == ModType::Extract || mod.type == ModType::TexturePackExtract || mod.type == ModType::ResourcePackExtract) {
            auto entry = ENV.metacache()->resolveEntry("ATLauncherPacks", cacheName);
            entry->setStale(true);
            modsToExtract.insert(entry->getFullPath(), mod);

            auto dl = Net::Download::makeCached(url, entry);
            jobPtr->addNetAction(dl);
        }
        else if(mod.type == ModType::Decomp) {
            auto entry = ENV.metacache()->resolveEntry("ATLauncherPacks", cacheName);
            entry->setStale(true);
            modsToDecomp.insert(entry->getFullPath(), mod);

            auto dl = Net::Download::makeCached(url, entry);
            jobPtr->addNetAction(dl);
        }
        else {
            auto relpath = getDirForModType(mod.type, mod.type_raw);
            if(relpath == Q_NULLPTR) continue;

            auto entry = ENV.metacache()->resolveEntry("ATLauncherPacks", cacheName);
            entry->setStale(true);

            auto dl = Net::Download::makeCached(url, entry);
            jobPtr->addNetAction(dl);

            auto path = FS::PathCombine(m_stagingPath, "minecraft", relpath, mod.file);
            qDebug() << "Will download" << url << "to" << path;
            modsToCopy[entry->getFullPath()] = path;

            if(mod.type == ModType::Forge) {
                auto vlist = ENV.metadataIndex()->get("net.minecraftforge");
                if(vlist)
                {
                    auto ver = vlist->getVersion(mod.version);
                    if(ver) {
                        ver->load(Net::Mode::Online);
                        componentsToInstall.insert("net.minecraftforge", ver);
                        continue;
                    }
                }

                qDebug() << "Jarmod: " + path;
                jarmods.push_back(path);
            }

            if(mod.type == ModType::Jar) {
                qDebug() << "Jarmod: " + path;
                jarmods.push_back(path);
            }
        }
    }

    connect(jobPtr.get(), &NetJob::succeeded, this, &PackInstallTask::onModsDownloaded);
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        jobPtr.reset();
        emitFailed(reason);
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        setProgress(current, total);
    });

    jobPtr->start();
}

void PackInstallTask::onModsDownloaded() {
    qDebug() << "PackInstallTask::onModsDownloaded: " << QThread::currentThreadId();
    jobPtr.reset();

    if(!modsToExtract.empty() || !modsToDecomp.empty() || !modsToCopy.empty()) {
        m_modExtractFuture = QtConcurrent::run(QThreadPool::globalInstance(), this, &PackInstallTask::extractMods, modsToExtract, modsToDecomp, modsToCopy);
        connect(&m_modExtractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &PackInstallTask::onModsExtracted);
        connect(&m_modExtractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, [&]()
        {
            emitAborted();
        });
        m_modExtractFutureWatcher.setFuture(m_modExtractFuture);
    }
    else {
        install();
    }
}

void PackInstallTask::onModsExtracted() {
    qDebug() << "PackInstallTask::onModsExtracted: " << QThread::currentThreadId();
    if(m_modExtractFuture.result()) {
        install();
    }
    else {
        emitFailed(tr("Failed to extract mods..."));
    }
}

bool PackInstallTask::extractMods(
    const QMap<QString, VersionMod> &toExtract,
    const QMap<QString, VersionMod> &toDecomp,
    const QMap<QString, QString> &toCopy
) {
    qDebug() << "PackInstallTask::extractMods: " << QThread::currentThreadId();

    setStatus(tr("Extracting mods..."));
    for (auto iter = toExtract.begin(); iter != toExtract.end(); iter++) {
        auto &modPath = iter.key();
        auto &mod = iter.value();

        QString extractToDir;
        if(mod.type == ModType::Extract) {
            extractToDir = getDirForModType(mod.extractTo, mod.extractTo_raw);
        }
        else if(mod.type == ModType::TexturePackExtract) {
            extractToDir = FS::PathCombine("texturepacks", "extracted");
        }
        else if(mod.type == ModType::ResourcePackExtract) {
            extractToDir = FS::PathCombine("resourcepacks", "extracted");
        }

        QDir extractDir(m_stagingPath);
        auto extractToPath = FS::PathCombine(extractDir.absolutePath(), "minecraft", extractToDir);

        QString folderToExtract = "";
        if(mod.type == ModType::Extract) {
            folderToExtract = mod.extractFolder;
            folderToExtract.remove(QRegExp("^/"));
        }

        qDebug() << "Extracting " + mod.file + " to " + extractToDir;
        if(!MMCZip::extractDir(modPath, folderToExtract, extractToPath)) {
            // assume error
            return false;
        }
    }

    for (auto iter = toDecomp.begin(); iter != toDecomp.end(); iter++) {
        auto &modPath = iter.key();
        auto &mod = iter.value();
        auto extractToDir = getDirForModType(mod.decompType, mod.decompType_raw);

        QDir extractDir(m_stagingPath);
        auto extractToPath = FS::PathCombine(extractDir.absolutePath(), "minecraft", extractToDir, mod.decompFile);

        qDebug() << "Extracting " + mod.decompFile + " to " + extractToDir;
        if(!MMCZip::extractFile(modPath, mod.decompFile, extractToPath)) {
            qWarning() << "Failed to extract" << mod.decompFile;
            return false;
        }
    }

    for (auto iter = toCopy.begin(); iter != toCopy.end(); iter++) {
        auto &from = iter.key();
        auto &to = iter.value();
        FS::copy fileCopyOperation(from, to);
        if(!fileCopyOperation()) {
            qWarning() << "Failed to copy" << from << "to" << to;
            return false;
        }
    }
    return true;
}

void PackInstallTask::install()
{
    qDebug() << "PackInstallTask::install: " << QThread::currentThreadId();
    setStatus(tr("Installing modpack"));

    auto instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->suspendSave();
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    // Use a component to add libraries BEFORE Minecraft
    if(!createLibrariesComponent(instance.instanceRoot(), components)) {
        emitFailed(tr("Failed to create libraries component"));
        return;
    }

    // Minecraft
    components->setComponentVersion("net.minecraft", m_version.minecraft, true);

    // Loader
    if(m_version.loader.type == QString("forge"))
    {
        auto version = getVersionForLoader("net.minecraftforge");
        if(version == Q_NULLPTR) return;

        components->setComponentVersion("net.minecraftforge", version, true);
    }
    else if(m_version.loader.type == QString("fabric"))
    {
        auto version = getVersionForLoader("net.fabricmc.fabric-loader");
        if(version == Q_NULLPTR) return;

        components->setComponentVersion("net.fabricmc.fabric-loader", version, true);
    }
    else if(m_version.loader.type != QString())
    {
        emitFailed(tr("Unknown loader type: ") + m_version.loader.type);
        return;
    }

    for(const auto & componentUid : componentsToInstall.keys()) {
        auto version = componentsToInstall.value(componentUid);
        components->setComponentVersion(componentUid, version->version());
    }

    components->installJarMods(jarmods);

    // Use a component to fill in the rest of the data
    // todo: use more detection
    if(!createPackComponent(instance.instanceRoot(), components)) {
        emitFailed(tr("Failed to create pack component"));
        return;
    }

    components->saveNow();

    instance.setName(m_instName);
    instance.setIconKey(m_instIcon);
    instanceSettings->resumeSave();

    jarmods.clear();
    emitSucceeded();
}

}
