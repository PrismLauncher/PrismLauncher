#include "ModrinthInstanceCreationTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "Json.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "net/NetJob.h"
#include "net/ChecksumValidator.h"

#include "settings/INISettingsObject.h"

#include "ui/dialogs/CustomMessageBox.h"

bool ModrinthCreationTask::createInstance()
{
    QEventLoop loop;

    if (m_files.empty() && !parseManifest())
        return false;

    auto mcPath = FS::PathCombine(m_stagingPath, ".minecraft");

    auto override_path = FS::PathCombine(m_stagingPath, "overrides");
    if (QFile::exists(override_path)) {
        if (!QFile::rename(override_path, mcPath)) {
            setError(tr("Could not rename the overrides folder:\n") + "overrides");
            return false;
        }
    }

    // Do client overrides
    auto client_override_path = FS::PathCombine(m_stagingPath, "client-overrides");
    if (QFile::exists(client_override_path)) {
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
    instance.setName(m_instName);
    instance.setManagedPack("modrinth", getManagedPackID(), m_managed_name, m_managed_id, {});
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

    return ended_well;
}

bool ModrinthCreationTask::parseManifest()
{
    try {
        QString indexPath = FS::PathCombine(m_stagingPath, "modrinth.index.json");
        auto doc = Json::requireDocument(indexPath);
        auto obj = Json::requireObject(doc, "modrinth.index.json");
        int formatVersion = Json::requireInteger(obj, "formatVersion", "modrinth.index.json");
        if (formatVersion == 1) {
            auto game = Json::requireString(obj, "game", "modrinth.index.json");
            if (game != "minecraft") {
                throw JSONValidationError("Unknown game: " + game);
            }

            m_managed_version_id = Json::ensureString(obj, "versionId", "Managed ID");
            m_managed_name = Json::ensureString(obj, "name", "Managed Name");

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
                        if (!had_optional) {
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

                m_files.push_back(file);
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
        QFile::remove(indexPath);
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
        return regex.match(m_source_url).captured(0);
    }

    return {};
}
