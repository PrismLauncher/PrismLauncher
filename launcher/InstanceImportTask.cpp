/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "InstanceImportTask.h"
#include "BaseInstance.h"
#include "FileSystem.h"
#include "Application.h"
#include "MMCZip.h"
#include "NullInstance.h"
#include "settings/INISettingsObject.h"
#include "icons/IconUtils.h"
#include <QtConcurrentRun>

// FIXME: this does not belong here, it's Minecraft/Flame specific
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "modplatform/flame/PackManifest.h"
#include "Json.h"
#include <quazip/quazipdir.h>
#include "modplatform/technic/TechnicPackProcessor.h"

#include "icons/IconList.h"
#include "Application.h"

InstanceImportTask::InstanceImportTask(const QUrl sourceUrl)
{
    m_sourceUrl = sourceUrl;
}

void InstanceImportTask::executeTask()
{
    if (m_sourceUrl.isLocalFile())
    {
        m_archivePath = m_sourceUrl.toLocalFile();
        processZipPack();
    }
    else
    {
        setStatus(tr("Downloading modpack:\n%1").arg(m_sourceUrl.toString()));
        m_downloadRequired = true;

        const QString path = m_sourceUrl.host() + '/' + m_sourceUrl.path();
        auto entry = APPLICATION->metacache()->resolveEntry("general", path);
        entry->setStale(true);
        m_filesNetJob = new NetJob(tr("Modpack download"), APPLICATION->network());
        m_filesNetJob->addNetAction(Net::Download::makeCached(m_sourceUrl, entry));
        m_archivePath = entry->getFullPath();
        auto job = m_filesNetJob.get();
        connect(job, &NetJob::succeeded, this, &InstanceImportTask::downloadSucceeded);
        connect(job, &NetJob::progress, this, &InstanceImportTask::downloadProgressChanged);
        connect(job, &NetJob::failed, this, &InstanceImportTask::downloadFailed);
        m_filesNetJob->start();
    }
}

void InstanceImportTask::downloadSucceeded()
{
    processZipPack();
    m_filesNetJob.reset();
}

void InstanceImportTask::downloadFailed(QString reason)
{
    emitFailed(reason);
    m_filesNetJob.reset();
}

void InstanceImportTask::downloadProgressChanged(qint64 current, qint64 total)
{
    setProgress(current / 2, total);
}

void InstanceImportTask::processZipPack()
{
    setStatus(tr("Extracting modpack"));
    QDir extractDir(m_stagingPath);
    qDebug() << "Attempting to create instance from" << m_archivePath;

    // open the zip and find relevant files in it
    m_packZip.reset(new QuaZip(m_archivePath));
    if (!m_packZip->open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Unable to open supplied modpack zip file."));
        return;
    }

    QStringList blacklist = {"instance.cfg", "manifest.json"};
    QString mmcFound = MMCZip::findFolderOfFileInZip(m_packZip.get(), "instance.cfg");
    bool technicFound = QuaZipDir(m_packZip.get()).exists("/bin/modpack.jar") || QuaZipDir(m_packZip.get()).exists("/bin/version.json");
    QString flameFound = MMCZip::findFolderOfFileInZip(m_packZip.get(), "manifest.json");
    QString root;
    if(!mmcFound.isNull())
    {
        // process as MultiMC instance/pack
        qDebug() << "MultiMC:" << mmcFound;
        root = mmcFound;
        m_modpackType = ModpackType::MultiMC;
    }
    else if (technicFound)
    {
        // process as Technic pack
        qDebug() << "Technic:" << technicFound;
        extractDir.mkpath(".minecraft");
        extractDir.cd(".minecraft");
        m_modpackType = ModpackType::Technic;
    }
    else if(!flameFound.isNull())
    {
        // process as Flame pack
        qDebug() << "Flame:" << flameFound;
        root = flameFound;
        m_modpackType = ModpackType::Flame;
    }
    if(m_modpackType == ModpackType::Unknown)
    {
        emitFailed(tr("Archive does not contain a recognized modpack type."));
        return;
    }

    // make sure we extract just the pack
    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractSubDir, m_packZip.get(), root, extractDir.absolutePath());
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &InstanceImportTask::extractFinished);
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &InstanceImportTask::extractAborted);
    m_extractFutureWatcher.setFuture(m_extractFuture);
}

void InstanceImportTask::extractFinished()
{
    m_packZip.reset();
    if (!m_extractFuture.result())
    {
        emitFailed(tr("Failed to extract modpack"));
        return;
    }
    QDir extractDir(m_stagingPath);

    qDebug() << "Fixing permissions for extracted pack files...";
    QDirIterator it(extractDir, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        auto filepath = it.next();
        QFileInfo file(filepath);
        auto permissions = QFile::permissions(filepath);
        auto origPermissions = permissions;
        if(file.isDir())
        {
            // Folder +rwx for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser;
        }
        else
        {
            // File +rw for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;
        }
        if(origPermissions != permissions)
        {
            if(!QFile::setPermissions(filepath, permissions))
            {
                logWarning(tr("Could not fix permissions for %1").arg(filepath));
            }
            else
            {
                qDebug() << "Fixed" << filepath;
            }
        }
    }

    switch(m_modpackType)
    {
        case ModpackType::Flame:
            processFlame();
            return;
        case ModpackType::MultiMC:
            processMultiMC();
            return;
        case ModpackType::Technic:
            processTechnic();
            return;
        case ModpackType::Unknown:
            emitFailed(tr("Archive does not contain a recognized modpack type."));
            return;
    }
}

void InstanceImportTask::extractAborted()
{
    emitFailed(tr("Instance import has been aborted."));
    return;
}

void InstanceImportTask::processFlame()
{
    const static QMap<QString,QString> forgemap = {
        {"1.2.5", "3.4.9.171"},
        {"1.4.2", "6.0.1.355"},
        {"1.4.7", "6.6.2.534"},
        {"1.5.2", "7.8.1.737"}
    };
    Flame::Manifest pack;
    try
    {
        QString configPath = FS::PathCombine(m_stagingPath, "manifest.json");
        Flame::loadManifest(pack, configPath);
        QFile::remove(configPath);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }
    if(!pack.overrides.isEmpty())
    {
        QString overridePath = FS::PathCombine(m_stagingPath, pack.overrides);
        if (QFile::exists(overridePath))
        {
            QString mcPath = FS::PathCombine(m_stagingPath, "minecraft");
            if (!QFile::rename(overridePath, mcPath))
            {
                emitFailed(tr("Could not rename the overrides folder:\n") + pack.overrides);
                return;
            }
        }
        else
        {
            logWarning(tr("The specified overrides folder (%1) is missing. Maybe the modpack was already used before?").arg(pack.overrides));
        }
    }

    QString forgeVersion;
    QString fabricVersion;
    for(auto &loader: pack.minecraft.modLoaders)
    {
        auto id = loader.id;
        if(id.startsWith("forge-"))
        {
            id.remove("forge-");
            forgeVersion = id;
            continue;
        }
        if(id.startsWith("fabric-"))
        {
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
    if(mcVersion.endsWith('.'))
    {
        mcVersion.remove(QRegExp("[.]+$"));
        logWarning(tr("Mysterious trailing dots removed from Minecraft version while importing pack."));
    }
    auto components = instance.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", mcVersion, true);
    if(!forgeVersion.isEmpty())
    {
        // FIXME: dirty, nasty, hack. Proper solution requires dependency resolution and knowledge of the metadata.
        if(forgeVersion == "recommended")
        {
            if(forgemap.contains(mcVersion))
            {
                forgeVersion = forgemap[mcVersion];
            }
            else
            {
                logWarning(tr("Could not map recommended forge version for Minecraft %1").arg(mcVersion));
            }
        }
        components->setComponentVersion("net.minecraftforge", forgeVersion);
    }
    if(!fabricVersion.isEmpty())
    {
        components->setComponentVersion("net.fabricmc.fabric-loader", fabricVersion);
    }
    if (m_instIcon != "default")
    {
        instance.setIconKey(m_instIcon);
    }
    else
    {
        if(pack.name.contains("Direwolf20"))
        {
            instance.setIconKey("steve");
        }
        else if(pack.name.contains("FTB") || pack.name.contains("Feed The Beast"))
        {
            instance.setIconKey("ftb_logo");
        }
        else
        {
            // default to something other than the MultiMC default to distinguish these
            instance.setIconKey("flame");
        }
    }
    QString jarmodsPath = FS::PathCombine(m_stagingPath, "minecraft", "jarmods");
    QFileInfo jarmodsInfo(jarmodsPath);
    if(jarmodsInfo.isDir())
    {
        // install all the jar mods
        qDebug() << "Found jarmods:";
        QDir jarmodsDir(jarmodsPath);
        QStringList jarMods;
        for (auto info: jarmodsDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files))
        {
            qDebug() << info.fileName();
            jarMods.push_back(info.absoluteFilePath());
        }
        auto profile = instance.getPackProfile();
        profile->installJarMods(jarMods);
        // nuke the original files
        FS::deletePath(jarmodsPath);
    }
    instance.setName(m_instName);
    m_modIdResolver = new Flame::FileResolvingTask(APPLICATION->network(), pack);
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::succeeded, [&]()
    {
        auto results = m_modIdResolver->getResults();
        m_filesNetJob = new NetJob(tr("Mod download"), APPLICATION->network());
        for(auto result: results.files)
        {
            QString filename = result.fileName;
            if(!result.required)
            {
                filename += ".disabled";
            }

            auto relpath = FS::PathCombine("minecraft", result.targetFolder, filename);
            auto path = FS::PathCombine(m_stagingPath , relpath);

            switch(result.type)
            {
                case Flame::File::Type::Folder:
                {
                    logWarning(tr("This 'Folder' may need extracting: %1").arg(relpath));
                    // fall-through intentional, we treat these as plain old mods and dump them wherever.
                }
                case Flame::File::Type::SingleFile:
                case Flame::File::Type::Mod:
                {
                    qDebug() << "Will download" << result.url << "to" << path;
                    auto dl = Net::Download::makeFile(result.url, path);
                    m_filesNetJob->addNetAction(dl);
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
        m_modIdResolver.reset();
        connect(m_filesNetJob.get(), &NetJob::succeeded, this, [&]()
        {
            m_filesNetJob.reset();
            emitSucceeded();
        }
        );
        connect(m_filesNetJob.get(), &NetJob::failed, [&](QString reason)
        {
            m_filesNetJob.reset();
            emitFailed(reason);
        });
        connect(m_filesNetJob.get(), &NetJob::progress, [&](qint64 current, qint64 total)
        {
            setProgress(current, total);
        });
        setStatus(tr("Downloading mods..."));
        m_filesNetJob->start();
    }
    );
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::failed, [&](QString reason)
    {
        m_modIdResolver.reset();
        emitFailed(tr("Unable to resolve mod IDs:\n") + reason);
    });
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::progress, [&](qint64 current, qint64 total)
    {
        setProgress(current, total);
    });
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::status, [&](QString status)
    {
        setStatus(status);
    });
    m_modIdResolver->start();
}

void InstanceImportTask::processTechnic()
{
    shared_qobject_ptr<Technic::TechnicPackProcessor> packProcessor = new Technic::TechnicPackProcessor();
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &InstanceImportTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &InstanceImportTask::emitFailed);
    packProcessor->run(m_globalSettings, m_instName, m_instIcon, m_stagingPath);
}

void InstanceImportTask::processMultiMC()
{
    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);

    NullInstance instance(m_globalSettings, instanceSettings, m_stagingPath);

    // reset time played on import... because packs.
    instance.resetTimePlayed();

    // set a new nice name
    instance.setName(m_instName);

    // if the icon was specified by user, use that. otherwise pull icon from the pack
    if (m_instIcon != "default")
    {
        instance.setIconKey(m_instIcon);
    }
    else
    {
        m_instIcon = instance.iconKey();

        auto importIconPath = IconUtils::findBestIconIn(instance.instanceRoot(), m_instIcon);
        if (!importIconPath.isNull() && QFile::exists(importIconPath))
        {
            // import icon
            auto iconList = APPLICATION->icons();
            if (iconList->iconFileExists(m_instIcon))
            {
                iconList->deleteIcon(m_instIcon);
            }
            iconList->installIcons({importIconPath});
        }
    }
    emitSucceeded();
}
