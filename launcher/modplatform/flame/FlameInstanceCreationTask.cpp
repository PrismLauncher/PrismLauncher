#include "FlameInstanceCreationTask.h"

#include "modplatform/flame/PackManifest.h"

#include "Application.h"
#include "FileSystem.h"
#include "Json.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "settings/INISettingsObject.h"

#include "ui/dialogs/BlockedModsDialog.h"

bool FlameCreationTask::abort()
{
    if (m_files_job)
        m_files_job->abort();
    if (m_mod_id_resolver)
        m_mod_id_resolver->abort();

    return true;
}

const static QMap<QString, QString> forgemap = { { "1.2.5", "3.4.9.171" },
                                                 { "1.4.2", "6.0.1.355" },
                                                 { "1.4.7", "6.6.2.534" },
                                                 { "1.5.2", "7.8.1.737" } };

bool FlameCreationTask::createInstance()
{
    QEventLoop loop;

    Flame::Manifest pack;
    try {
        QString configPath = FS::PathCombine(m_stagingPath, "manifest.json");
        Flame::loadManifest(pack, configPath);
        QFile::remove(configPath);
    } catch (const JSONValidationError& e) {
        setError(tr("Could not understand pack manifest:\n") + e.cause());
        return false;
    }

    if (!pack.overrides.isEmpty()) {
        QString overridePath = FS::PathCombine(m_stagingPath, pack.overrides);
        if (QFile::exists(overridePath)) {
            QString mcPath = FS::PathCombine(m_stagingPath, "minecraft");
            if (!QFile::rename(overridePath, mcPath)) {
                setError(tr("Could not rename the overrides folder:\n") + pack.overrides);
                return false;
            }
        } else {
            logWarning(
                tr("The specified overrides folder (%1) is missing. Maybe the modpack was already used before?").arg(pack.overrides));
        }
    }

    QString forgeVersion;
    QString fabricVersion;
    // TODO: is Quilt relevant here?
    for (auto& loader : pack.minecraft.modLoaders) {
        auto id = loader.id;
        if (id.startsWith("forge-")) {
            id.remove("forge-");
            forgeVersion = id;
            continue;
        }
        if (id.startsWith("fabric-")) {
            id.remove("fabric-");
            fabricVersion = id;
            continue;
        }
        logWarning(tr("Unknown mod loader in manifest: %1").arg(id));
    }

    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto mcVersion = pack.minecraft.version;

    // Hack to correct some 'special sauce'...
    if (mcVersion.endsWith('.')) {
        mcVersion.remove(QRegularExpression("[.]+$"));
        logWarning(tr("Mysterious trailing dots removed from Minecraft version while importing pack."));
    }

    auto components = instance.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", mcVersion, true);
    if (!forgeVersion.isEmpty()) {
        // FIXME: dirty, nasty, hack. Proper solution requires dependency resolution and knowledge of the metadata.
        if (forgeVersion == "recommended") {
            if (forgemap.contains(mcVersion)) {
                forgeVersion = forgemap[mcVersion];
            } else {
                logWarning(tr("Could not map recommended Forge version for Minecraft %1").arg(mcVersion));
            }
        }
        components->setComponentVersion("net.minecraftforge", forgeVersion);
    }
    if (!fabricVersion.isEmpty())
        components->setComponentVersion("net.fabricmc.fabric-loader", fabricVersion);

    if (m_instIcon != "default") {
        instance.setIconKey(m_instIcon);
    } else {
        if (pack.name.contains("Direwolf20")) {
            instance.setIconKey("steve");
        } else if (pack.name.contains("FTB") || pack.name.contains("Feed The Beast")) {
            instance.setIconKey("ftb_logo");
        } else {
            instance.setIconKey("flame");
        }
    }

    QString jarmodsPath = FS::PathCombine(m_stagingPath, "minecraft", "jarmods");
    QFileInfo jarmodsInfo(jarmodsPath);
    if (jarmodsInfo.isDir()) {
        // install all the jar mods
        qDebug() << "Found jarmods:";
        QDir jarmodsDir(jarmodsPath);
        QStringList jarMods;
        for (auto info : jarmodsDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
            qDebug() << info.fileName();
            jarMods.push_back(info.absoluteFilePath());
        }
        auto profile = instance.getPackProfile();
        profile->installJarMods(jarMods);
        // nuke the original files
        FS::deletePath(jarmodsPath);
    }

    instance.setName(m_instName);

    m_mod_id_resolver = new Flame::FileResolvingTask(APPLICATION->network(), pack);
    connect(m_mod_id_resolver.get(), &Flame::FileResolvingTask::succeeded, this, [this, &loop]{
            idResolverSucceeded(loop);
    });
    connect(m_mod_id_resolver.get(), &Flame::FileResolvingTask::failed, [&](QString reason) {
        m_mod_id_resolver.reset();
        setError(tr("Unable to resolve mod IDs:\n") + reason);
    });
    connect(m_mod_id_resolver.get(), &Flame::FileResolvingTask::progress, this, &FlameCreationTask::setProgress);
    connect(m_mod_id_resolver.get(), &Flame::FileResolvingTask::status, this, &FlameCreationTask::setStatus);

    m_mod_id_resolver->start();

    loop.exec();

    return getError().isEmpty();
}

void FlameCreationTask::idResolverSucceeded(QEventLoop& loop)
{
    auto results = m_mod_id_resolver->getResults();

    // first check for blocked mods
    QString text;
    QList<QUrl> urls;
    auto anyBlocked = false;
    for (const auto& result : results.files.values()) {
        if (!result.resolved || result.url.isEmpty()) {
            text += QString("%1: <a href='%2'>%2</a><br/>").arg(result.fileName, result.websiteUrl);
            urls.append(QUrl(result.websiteUrl));
            anyBlocked = true;
        }
    }
    if (anyBlocked) {
        qWarning() << "Blocked mods found, displaying mod list";

        auto message_dialog = new BlockedModsDialog(m_parent, tr("Blocked mods found"),
                                                   tr("The following mods were blocked on third party launchers.<br/>"
                                                      "You will need to manually download them and add them to the modpack"),
                                                   text,
                                                   urls);
        message_dialog->setModal(true);

        if (message_dialog->exec()) {
            setupDownloadJob(loop);
        } else {
            m_mod_id_resolver.reset();
            setError("Canceled");
        }
    } else {
        setupDownloadJob(loop);
    }
}

void FlameCreationTask::setupDownloadJob(QEventLoop& loop)
{
    m_files_job = new NetJob(tr("Mod download"), APPLICATION->network());
    for (const auto& result : m_mod_id_resolver->getResults().files) {
        QString filename = result.fileName;
        if (!result.required) {
            filename += ".disabled";
        }

        auto relpath = FS::PathCombine("minecraft", result.targetFolder, filename);
        auto path = FS::PathCombine(m_stagingPath, relpath);

        switch (result.type) {
            case Flame::File::Type::Folder: {
                logWarning(tr("This 'Folder' may need extracting: %1").arg(relpath));
                // fall-through intentional, we treat these as plain old mods and dump them wherever.
            }
            case Flame::File::Type::SingleFile:
            case Flame::File::Type::Mod: {
                if (!result.url.isEmpty()) {
                    qDebug() << "Will download" << result.url << "to" << path;
                    auto dl = Net::Download::makeFile(result.url, path);
                    m_files_job->addNetAction(dl);
                }
                break;
            }
            case Flame::File::Type::Modpack:
                logWarning(tr("Nesting modpacks in modpacks is not implemented, nothing was downloaded: %1").arg(relpath));
                break;
            case Flame::File::Type::Cmod2:
            case Flame::File::Type::Ctoc:
            case Flame::File::Type::Unknown:
                logWarning(tr("Unrecognized/unhandled PackageType for: %1").arg(relpath));
                break;
        }
    }

    m_mod_id_resolver.reset();
    connect(m_files_job.get(), &NetJob::succeeded, this, [&]() {
        m_files_job.reset();
        emitSucceeded();
    });
    connect(m_files_job.get(), &NetJob::failed, [&](QString reason) {
        m_files_job.reset();
        setError(reason);
    });
    connect(m_files_job.get(), &NetJob::progress, [&](qint64 current, qint64 total) { setProgress(current, total); });
    connect(m_files_job.get(), &NetJob::finished, &loop, &QEventLoop::quit);

    setStatus(tr("Downloading mods..."));
    m_files_job->start();
}
