#include "FlameInstanceCreationTask.h"

#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/PackManifest.h"

#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "Json.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "modplatform/helpers/OverrideUtils.h"

#include "settings/INISettingsObject.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/BlockedModsDialog.h"

const static QMap<QString, QString> forgemap = { { "1.2.5", "3.4.9.171" },
                                                 { "1.4.2", "6.0.1.355" },
                                                 { "1.4.7", "6.6.2.534" },
                                                 { "1.5.2", "7.8.1.737" } };

static const FlameAPI api;

bool FlameCreationTask::abort()
{
    if (!canAbort())
        return false;

    m_abort = true;
    if (m_process_update_file_info_job)
        m_process_update_file_info_job->abort();
    if (m_files_job)
        m_files_job->abort();
    if (m_mod_id_resolver)
        m_mod_id_resolver->abort();

    return Task::abort();
}

bool FlameCreationTask::updateInstance()
{
    auto instance_list = APPLICATION->instances();

    // FIXME: How to handle situations when there's more than one install already for a given modpack?
    auto inst = instance_list->getInstanceByManagedName(originalName());

    if (!inst) {
        inst = instance_list->getInstanceById(originalName());

        if (!inst)
            return false;
    }

    QString index_path(FS::PathCombine(m_stagingPath, "manifest.json"));

    try {
        Flame::loadManifest(m_pack, index_path);
    } catch (const JSONValidationError& e) {
        setError(tr("Could not understand pack manifest:\n") + e.cause());
        return false;
    }

    auto version_id = inst->getManagedPackVersionName();
    auto version_str = !version_id.isEmpty() ? tr(" (version %1)").arg(version_id) : "";

    auto info = CustomMessageBox::selectable(
        m_parent, tr("Similar modpack was found!"),
        tr("One or more of your instances are from this same modpack%1. Do you want to create a "
           "separate instance, or update the existing one?\n\nNOTE: Make sure you made a backup of your important instance data before "
           "updating, as worlds can be corrupted and some configuration may be lost (due to pack overrides).")
            .arg(version_str), QMessageBox::Information, QMessageBox::Ok | QMessageBox::Reset | QMessageBox::Abort);
    info->setButtonText(QMessageBox::Ok, tr("Update existing instance"));
    info->setButtonText(QMessageBox::Abort, tr("Create new instance"));
    info->setButtonText(QMessageBox::Reset, tr("Cancel"));

    info->exec();

    if (info->clickedButton() == info->button(QMessageBox::Abort))
        return false;

    if (info->clickedButton() == info->button(QMessageBox::Reset)) {
        m_abort = true;
        return false;
    }

    QDir old_inst_dir(inst->instanceRoot());

    QString old_index_folder(FS::PathCombine(old_inst_dir.absolutePath(), "flame"));
    QString old_index_path(FS::PathCombine(old_index_folder, "manifest.json"));

    QFileInfo old_index_file(old_index_path);
    if (old_index_file.exists()) {
        Flame::Manifest old_pack;
        Flame::loadManifest(old_pack, old_index_path);

        auto& old_files = old_pack.files;

        auto& files = m_pack.files;

        // Remove repeated files, we don't need to download them!
        auto files_iterator = files.begin();
        while (files_iterator != files.end()) {
            auto const& file = files_iterator;

            auto old_file = old_files.find(file.key());
            if (old_file != old_files.end()) {
                // We found a match, but is it a different version?
                if (old_file->fileId == file->fileId) {
                    qDebug() << "Removed file at" << file->targetFolder << "with id" << file->fileId << "from list of downloads";

                    old_files.remove(file.key());
                    files_iterator = files.erase(files_iterator);
                }
            }

            files_iterator++;
        }

        QString old_minecraft_dir(inst->gameRoot());

        // We will remove all the previous overrides, to prevent duplicate files!
        // TODO: Currently 'overrides' will always override the stuff on update. How do we preserve unchanged overrides?
        // FIXME: We may want to do something about disabled mods.
        auto old_overrides = Override::readOverrides("overrides", old_index_folder);
        for (auto entry : old_overrides) {
            qDebug() << "Removing" << entry;
            old_minecraft_dir.remove(entry);
        }

        // Remove remaining old files (we need to do an API request to know which ids are which files...)
        QStringList fileIds;

        for (auto& file : old_files) {
            fileIds.append(QString::number(file.fileId));
        }

        auto* raw_response = new QByteArray;
        auto job = api.getFiles(fileIds, raw_response);

        QEventLoop loop;

        connect(job, &NetJob::succeeded, this, [raw_response, fileIds, old_inst_dir, &old_files, old_minecraft_dir] {
            // Parse the API response
            QJsonParseError parse_error{};
            auto doc = QJsonDocument::fromJson(*raw_response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from Flame files task at " << parse_error.offset
                           << " reason: " << parse_error.errorString();
                qWarning() << *raw_response;
                return;
            }

            try {
                QJsonArray entries;
                if (fileIds.size() == 1)
                    entries = { Json::requireObject(Json::requireObject(doc), "data") };
                else
                    entries = Json::requireArray(Json::requireObject(doc), "data");

                for (auto entry : entries) {
                    auto entry_obj = Json::requireObject(entry);

                    Flame::File file;
                    // We don't care about blocked mods, we just need local data to delete the file
                    file.parseFromObject(entry_obj, false);

                    auto id = Json::requireInteger(entry_obj, "id");
                    old_files.insert(id, file);
                }
            } catch (Json::JsonException& e) {
                qCritical() << e.cause() << e.what();
            }

            // Delete the files
            for (auto& file : old_files) {
                if (file.fileName.isEmpty() || file.targetFolder.isEmpty())
                    continue;

                qDebug() << "Removing" << file.fileName << "at" << file.targetFolder;
                QString path(FS::PathCombine(old_minecraft_dir, file.targetFolder, file.fileName));
                if (!QFile::remove(path))
                    qDebug() << "Failed to remove file at" << path;
            }
        });
        connect(job, &NetJob::finished, &loop, &QEventLoop::quit);

        m_process_update_file_info_job = job;
        job->start();

        loop.exec();

        m_process_update_file_info_job = nullptr;
    }

    setOverride(true);
    qDebug() << "Will override instance!";

    m_instance = inst;

    // We let it go through the createInstance() stage, just with a couple modifications for updating
    return false;
}

bool FlameCreationTask::createInstance()
{
    QEventLoop loop;

    QString parent_folder(FS::PathCombine(m_stagingPath, "flame"));

    try {
        QString index_path(FS::PathCombine(m_stagingPath, "manifest.json"));
        if (!m_pack.is_loaded)
            Flame::loadManifest(m_pack, index_path);

        // Keep index file in case we need it some other time (like when changing versions)
        QString new_index_place(FS::PathCombine(parent_folder, "manifest.json"));
        FS::ensureFilePathExists(new_index_place);
        QFile::rename(index_path, new_index_place);

    } catch (const JSONValidationError& e) {
        setError(tr("Could not understand pack manifest:\n") + e.cause());
        return false;
    }

    if (!m_pack.overrides.isEmpty()) {
        QString overridePath = FS::PathCombine(m_stagingPath, m_pack.overrides);
        if (QFile::exists(overridePath)) {
            // Create a list of overrides in "overrides.txt" inside flame/
            Override::createOverrides("overrides", parent_folder, overridePath);

            QString mcPath = FS::PathCombine(m_stagingPath, "minecraft");
            if (!QFile::rename(overridePath, mcPath)) {
                setError(tr("Could not rename the overrides folder:\n") + m_pack.overrides);
                return false;
            }
        } else {
            logWarning(
                tr("The specified overrides folder (%1) is missing. Maybe the modpack was already used before?").arg(m_pack.overrides));
        }
    }

    QString forgeVersion;
    QString fabricVersion;
    // TODO: is Quilt relevant here?
    for (auto& loader : m_pack.minecraft.modLoaders) {
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
    auto mcVersion = m_pack.minecraft.version;

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
        if (m_pack.name.contains("Direwolf20")) {
            instance.setIconKey("steve");
        } else if (m_pack.name.contains("FTB") || m_pack.name.contains("Feed The Beast")) {
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

    instance.setManagedPack("flame", {}, m_pack.name, {}, m_pack.version);
    instance.setName(name());

    m_mod_id_resolver = new Flame::FileResolvingTask(APPLICATION->network(), m_pack);
    connect(m_mod_id_resolver.get(), &Flame::FileResolvingTask::succeeded, this, [this, &loop] { idResolverSucceeded(loop); });
    connect(m_mod_id_resolver.get(), &Flame::FileResolvingTask::failed, [&](QString reason) {
        m_mod_id_resolver.reset();
        setError(tr("Unable to resolve mod IDs:\n") + reason);
    });
    connect(m_mod_id_resolver.get(), &Flame::FileResolvingTask::progress, this, &FlameCreationTask::setProgress);
    connect(m_mod_id_resolver.get(), &Flame::FileResolvingTask::status, this, &FlameCreationTask::setStatus);

    m_mod_id_resolver->start();

    loop.exec();

    if (m_instance) {
        auto inst = m_instance.value();

        inst->copyManagedPack(instance);
        inst->setName(instance.name());
    }

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
            loop.quit();
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
