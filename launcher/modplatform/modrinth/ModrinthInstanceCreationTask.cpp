#include "ModrinthInstanceCreationTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "Json.h"

#include "minecraft/PackProfile.h"

#include "modplatform/helpers/OverrideUtils.h"

#include "net/ChecksumValidator.h"

#include "settings/INISettingsObject.h"

#include "ui/dialogs/CustomMessageBox.h"

#include <QAbstractButton>

bool ModrinthCreationTask::abort()
{
    if (!canAbort())
        return false;

    m_abort = true;
    if (m_files_job)
        m_files_job->abort();
    return Task::abort();
}

bool ModrinthCreationTask::updateInstance()
{
    auto instance_list = APPLICATION->instances();

    // FIXME: How to handle situations when there's more than one install already for a given modpack?
    auto inst = instance_list->getInstanceByManagedName(originalName());

    if (!inst) {
        inst = instance_list->getInstanceById(originalName());

        if (!inst)
            return false;
    }

    QString index_path = FS::PathCombine(m_stagingPath, "modrinth.index.json");
    if (!parseManifest(index_path, m_files, true, false))
        return false;

    auto version_name = inst->getManagedPackVersionName();
    auto version_str = !version_name.isEmpty() ? tr(" (version %1)").arg(version_name) : "";

    auto info = CustomMessageBox::selectable(
        m_parent, tr("Similar modpack was found!"),
        tr("One or more of your instances are from this same modpack%1. Do you want to create a "
           "separate instance, or update the existing one?\n\nNOTE: Make sure you made a backup of your important instance data before "
           "updating, as worlds can be corrupted and some configuration may be lost (due to pack overrides).")
            .arg(version_str),
        QMessageBox::Information, QMessageBox::Ok | QMessageBox::Reset | QMessageBox::Abort);
    info->setButtonText(QMessageBox::Ok, tr("Create new instance"));
    info->setButtonText(QMessageBox::Abort, tr("Update existing instance"));
    info->setButtonText(QMessageBox::Reset, tr("Cancel"));

    info->exec();

    if (info->clickedButton() == info->button(QMessageBox::Ok))
        return false;

    if (info->clickedButton() == info->button(QMessageBox::Reset)) {
        m_abort = true;
        return false;
    }

    // Remove repeated files, we don't need to download them!
    QDir old_inst_dir(inst->instanceRoot());

    QString old_index_folder(FS::PathCombine(old_inst_dir.absolutePath(), "mrpack"));

    QString old_index_path(FS::PathCombine(old_index_folder, "modrinth.index.json"));
    QFileInfo old_index_file(old_index_path);
    if (old_index_file.exists()) {
        std::vector<Modrinth::File> old_files;
        parseManifest(old_index_path, old_files, false, false);

        // Let's remove all duplicated, identical resources!
        auto files_iterator = m_files.begin();
    begin:
        while (files_iterator != m_files.end()) {
            auto const& file = *files_iterator;

            auto old_files_iterator = old_files.begin();
            while (old_files_iterator != old_files.end()) {
                auto const& old_file = *old_files_iterator;

                if (old_file.hash == file.hash) {
                    qDebug() << "Removed file at" << file.path << "from list of downloads";
                    files_iterator = m_files.erase(files_iterator);
                    old_files_iterator = old_files.erase(old_files_iterator);
                    goto begin;  // Sorry :c
                }

                old_files_iterator++;
            }

            files_iterator++;
        }

        QDir old_minecraft_dir(inst->gameRoot());

        // Some files were removed from the old version, and some will be downloaded in an updated version,
        // so we're fine removing them!
        if (!old_files.empty()) {
            for (auto const& file : old_files) {
                if (file.path.isEmpty())
                    continue;
                qDebug() << "Scheduling" << file.path << "for removal";
                m_files_to_remove.append(old_minecraft_dir.absoluteFilePath(file.path));
            }
        }

        // We will remove all the previous overrides, to prevent duplicate files!
        // TODO: Currently 'overrides' will always override the stuff on update. How do we preserve unchanged overrides?
        // FIXME: We may want to do something about disabled mods.
        auto old_overrides = Override::readOverrides("overrides", old_index_folder);
        for (auto entry : old_overrides) {
            if (entry.isEmpty())
                continue;
            qDebug() << "Scheduling" << entry << "for removal";
            m_files_to_remove.append(old_minecraft_dir.absoluteFilePath(entry));
        }

        auto old_client_overrides = Override::readOverrides("client-overrides", old_index_folder);
        for (auto entry : old_overrides) {
            if (entry.isEmpty())
                continue;
            qDebug() << "Scheduling" << entry << "for removal";
            m_files_to_remove.append(old_minecraft_dir.absoluteFilePath(entry));
        }
    }


    setOverride(true);
    qDebug() << "Will override instance!";

    m_instance = inst;

    // We let it go through the createInstance() stage, just with a couple modifications for updating
    return false;
}

// https://docs.modrinth.com/docs/modpacks/format_definition/
bool ModrinthCreationTask::createInstance()
{
    QEventLoop loop;

    QString parent_folder(FS::PathCombine(m_stagingPath, "mrpack"));

    QString index_path = FS::PathCombine(m_stagingPath, "modrinth.index.json");
    if (m_files.empty() && !parseManifest(index_path, m_files, true, true))
        return false;

    // Keep index file in case we need it some other time (like when changing versions)
    QString new_index_place(FS::PathCombine(parent_folder, "modrinth.index.json"));
    FS::ensureFilePathExists(new_index_place);
    QFile::rename(index_path, new_index_place);

    auto mcPath = FS::PathCombine(m_stagingPath, ".minecraft");

    auto override_path = FS::PathCombine(m_stagingPath, "overrides");
    if (QFile::exists(override_path)) {
        // Create a list of overrides in "overrides.txt" inside mrpack/
        Override::createOverrides("overrides", parent_folder, override_path);

        // Apply the overrides
        if (!QFile::rename(override_path, mcPath)) {
            setError(tr("Could not rename the overrides folder:\n") + "overrides");
            return false;
        }
    }

    // Do client overrides
    auto client_override_path = FS::PathCombine(m_stagingPath, "client-overrides");
    if (QFile::exists(client_override_path)) {
        // Create a list of overrides in "client-overrides.txt" inside mrpack/
        Override::createOverrides("client-overrides", parent_folder, client_override_path);

        // Apply the overrides
        if (!FS::overrideFolder(mcPath, client_override_path)) {
            setError(tr("Could not rename the client overrides folder:\n") + "client overrides");
            return false;
        }
    }

    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);

    auto components = instance.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", minecraftVersion, true);

    if (!fabricVersion.isEmpty())
        components->setComponentVersion("net.fabricmc.fabric-loader", fabricVersion);
    if (!quiltVersion.isEmpty())
        components->setComponentVersion("org.quiltmc.quilt-loader", quiltVersion);
    if (!forgeVersion.isEmpty())
        components->setComponentVersion("net.minecraftforge", forgeVersion);

    if (m_instIcon != "default") {
        instance.setIconKey(m_instIcon);
    } else {
        instance.setIconKey("modrinth");
    }

    instance.setManagedPack("modrinth", getManagedPackID(), m_managed_name, m_managed_version_id, version());
    instance.setName(name());
    instance.saveNow();

    m_files_job = new NetJob(tr("Mod download"), APPLICATION->network());

    for (auto file : m_files) {
        auto path = FS::PathCombine(m_stagingPath, ".minecraft", file.path);
        qDebug() << "Will try to download" << file.downloads.front() << "to" << path;
        auto dl = Net::Download::makeFile(file.downloads.dequeue(), path);
        dl->addValidator(new Net::ChecksumValidator(file.hashAlgorithm, file.hash));
        m_files_job->addNetAction(dl);

        if (file.downloads.size() > 0) {
            // FIXME: This really needs to be put into a ConcurrentTask of
            // MultipleOptionsTask's , once those exist :)
            connect(dl.get(), &NetAction::failed, [this, &file, path, dl] {
                auto dl = Net::Download::makeFile(file.downloads.dequeue(), path);
                dl->addValidator(new Net::ChecksumValidator(file.hashAlgorithm, file.hash));
                m_files_job->addNetAction(dl);
                dl->succeeded();
            });
        }
    }

    bool ended_well = false;

    connect(m_files_job.get(), &NetJob::succeeded, this, [&]() { ended_well = true; });
    connect(m_files_job.get(), &NetJob::failed, [&](const QString& reason) {
        ended_well = false;
        setError(reason);
    });
    connect(m_files_job.get(), &NetJob::finished, &loop, &QEventLoop::quit);
    connect(m_files_job.get(), &NetJob::progress, [&](qint64 current, qint64 total) { setProgress(current, total); });

    setStatus(tr("Downloading mods..."));
    m_files_job->start();

    loop.exec();

    if (m_instance && ended_well) {
        setAbortStatus(false);
        auto inst = m_instance.value();

        inst->copyManagedPack(instance);
        inst->setName(instance.name());
    }

    return ended_well;
}

bool ModrinthCreationTask::parseManifest(QString index_path, std::vector<Modrinth::File>& files, bool set_managed_info, bool show_optional_dialog)
{
    try {
        auto doc = Json::requireDocument(index_path);
        auto obj = Json::requireObject(doc, "modrinth.index.json");
        int formatVersion = Json::requireInteger(obj, "formatVersion", "modrinth.index.json");
        if (formatVersion == 1) {
            auto game = Json::requireString(obj, "game", "modrinth.index.json");
            if (game != "minecraft") {
                throw JSONValidationError("Unknown game: " + game);
            }

            if (set_managed_info) {
                m_managed_version_id = Json::ensureString(obj, "versionId", {}, "Managed ID");
                m_managed_name = Json::ensureString(obj, "name", {}, "Managed Name");
            }

            auto jsonFiles = Json::requireIsArrayOf<QJsonObject>(obj, "files", "modrinth.index.json");
            bool had_optional = false;
            for (auto modInfo : jsonFiles) {
                Modrinth::File file;
                file.path = Json::requireString(modInfo, "path");

                auto env = Json::ensureObject(modInfo, "env");
                // 'env' field is optional
                if (!env.isEmpty()) {
                    QString support = Json::ensureString(env, "client", "unsupported");
                    if (support == "unsupported") {
                        continue;
                    } else if (support == "optional") {
                        // TODO: Make a review dialog for choosing which ones the user wants!
                        if (!had_optional && show_optional_dialog) {
                            had_optional = true;
                            auto info = CustomMessageBox::selectable(
                                m_parent, tr("Optional mod detected!"),
                                tr("One or more mods from this modpack are optional. They will be downloaded, but disabled by default!"),
                                QMessageBox::Information);
                            info->exec();
                        }

                        if (file.path.endsWith(".jar"))
                            file.path += ".disabled";
                    }
                }

                QJsonObject hashes = Json::requireObject(modInfo, "hashes");
                QString hash;
                QCryptographicHash::Algorithm hashAlgorithm;
                hash = Json::ensureString(hashes, "sha1");
                hashAlgorithm = QCryptographicHash::Sha1;
                if (hash.isEmpty()) {
                    hash = Json::ensureString(hashes, "sha512");
                    hashAlgorithm = QCryptographicHash::Sha512;
                    if (hash.isEmpty()) {
                        hash = Json::ensureString(hashes, "sha256");
                        hashAlgorithm = QCryptographicHash::Sha256;
                        if (hash.isEmpty()) {
                            throw JSONValidationError("No hash found for: " + file.path);
                        }
                    }
                }
                file.hash = QByteArray::fromHex(hash.toLatin1());
                file.hashAlgorithm = hashAlgorithm;

                // Do not use requireUrl, which uses StrictMode, instead use QUrl's default TolerantMode
                // (as Modrinth seems to incorrectly handle spaces)

                auto download_arr = Json::ensureArray(modInfo, "downloads");
                for (auto download : download_arr) {
                    qWarning() << download.toString();
                    bool is_last = download.toString() == download_arr.last().toString();

                    auto download_url = QUrl(download.toString());

                    if (!download_url.isValid()) {
                        qDebug()
                            << QString("Download URL (%1) for %2 is not a correctly formatted URL").arg(download_url.toString(), file.path);
                        if (is_last && file.downloads.isEmpty())
                            throw JSONValidationError(tr("Download URL for %1 is not a correctly formatted URL").arg(file.path));
                    } else {
                        file.downloads.push_back(download_url);
                    }
                }

                files.push_back(file);
            }

            auto dependencies = Json::requireObject(obj, "dependencies", "modrinth.index.json");
            for (auto it = dependencies.begin(), end = dependencies.end(); it != end; ++it) {
                QString name = it.key();
                if (name == "minecraft") {
                    minecraftVersion = Json::requireString(*it, "Minecraft version");
                } else if (name == "fabric-loader") {
                    fabricVersion = Json::requireString(*it, "Fabric Loader version");
                } else if (name == "quilt-loader") {
                    quiltVersion = Json::requireString(*it, "Quilt Loader version");
                } else if (name == "forge") {
                    forgeVersion = Json::requireString(*it, "Forge version");
                } else {
                    throw JSONValidationError("Unknown dependency type: " + name);
                }
            }
        } else {
            throw JSONValidationError(QStringLiteral("Unknown format version: %s").arg(formatVersion));
        }

    } catch (const JSONValidationError& e) {
        setError(tr("Could not understand pack index:\n") + e.cause());
        return false;
    }

    return true;
}

QString ModrinthCreationTask::getManagedPackID() const
{
    if (!m_source_url.isEmpty()) {
        QRegularExpression regex(R"(data\/(.*)\/versions)");
        return regex.match(m_source_url).captured(1);
    }

    return {};
}
