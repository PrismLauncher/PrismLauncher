// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "TechnicInstanceCreationTask.h"

#include <FileSystem.h>
#include <Json.h>
#include <MMCZip.h>

#include "Application.h"
#include "InstanceList.h"
#include "TechnicAPI.h"
#include "TechnicPackProcessor.h"
#include "minecraft/MinecraftInstance.h"
#include "net/ApiDownload.h"
#include "net/ChecksumValidator.h"
#include "settings/INISettingsObject.h"

#include "ui/dialogs/CustomMessageBox.h"

namespace Technic {

bool InstanceCreationTask::abort()
{
    if (m_abortable && m_filesNetJob) {
        m_abort = true;
        return m_filesNetJob->abort();
    }
    return false;
}

bool InstanceCreationTask::updateInstance()
{
    auto instance_list = APPLICATION->instances();

    BaseInstance* inst = nullptr;
    if (!m_original_instance_id.isEmpty()) {
        inst = instance_list->getInstanceById(m_original_instance_id);
        if (!inst) {
            // Try by managed name
            inst = instance_list->getInstanceByManagedName(originalName());
        }
    }

    if (!inst)
        return false;

    auto version_name = inst->getManagedPackVersionName();
    auto version_str = !version_name.isEmpty() ? tr(" (version %1)").arg(version_name) : "";

    if (shouldConfirmUpdate()) {
        auto should_update = askIfShouldUpdate(m_parent, version_str);
        if (should_update == ShouldUpdate::SkipUpdating)
            return false;
        if (should_update == ShouldUpdate::Cancel) {
            m_abort = true;
            return false;
        }
    }

    setOverride(true, inst->id());
    qDebug() << "Will override Technic instance!";

    m_instance = inst;

    // Continue to createInstance() for the actual update work
    return false;
}

bool InstanceCreationTask::createInstance()
{
    QEventLoop loop;

    if (m_isSolder) {
        // Fetch the build info from Solder
        setStatus(tr("Resolving modpack files..."));

        auto job = API::getSolderPackBuild(m_solderUrl, m_slug, m_version, m_response.get());
        connect(job.get(), &Task::succeeded, this, [this, &loop]() {
            QJsonParseError parse_error{};
            QJsonDocument doc = QJsonDocument::fromJson(*m_response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error parsing Solder build response:" << parse_error.errorString();
                setError(tr("Could not parse Solder build response."));
                loop.quit();
                return;
            }

            try {
                TechnicPlatform::SolderBuild build;
                TechnicPlatform::loadSolderBuild(build, doc.object());

                if (!build.minecraft.isEmpty())
                    m_minecraftVersion = build.minecraft;

                // Download all mods
                setStatus(tr("Downloading modpack..."));
                m_filesNetJob.reset(new NetJob(tr("Downloading modpack"), APPLICATION->network()));

                int i = 0;
                for (const auto& mod : build.mods) {
                    auto path = FS::PathCombine(m_outputDir.path(), QString("%1").arg(i));

                    auto dl = Net::ApiDownload::makeFile(mod.url, path);
                    if (!mod.md5.isEmpty()) {
                        dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Md5, QByteArray::fromHex(mod.md5.toLatin1())));
                    }
                    m_filesNetJob->addNetAction(dl);
                    i++;
                }

                m_modCount = build.mods.size();

                connect(m_filesNetJob.get(), &NetJob::succeeded, this, [this, &loop]() {
                    m_abortable = false;
                    extractMods();
                    loop.quit();
                });
                connect(m_filesNetJob.get(), &NetJob::failed, this, [this, &loop](QString reason) {
                    m_abortable = false;
                    m_filesNetJob.reset();
                    setError(reason);
                    loop.quit();
                });
                connect(m_filesNetJob.get(), &NetJob::aborted, this, [this, &loop]() {
                    m_filesNetJob.reset();
                    m_abort = true;
                    loop.quit();
                });
                connect(m_filesNetJob.get(), &NetJob::progress, this, [this](qint64 current, qint64 total) {
                    m_abortable = true;
                    setProgress(current / 2, total);
                });
                connect(m_filesNetJob.get(), &NetJob::stepProgress, this, &InstanceCreationTask::propagateStepProgress);

                m_filesNetJob->start();

            } catch (const JSONValidationError& e) {
                setError(tr("Could not understand Solder build response:\n") + e.cause());
                loop.quit();
                return;
            }
        });

        connect(job.get(), &Task::failed, this, [this, &loop](QString reason) {
            setError(reason);
            loop.quit();
        });

        job->start();
        loop.exec();

    } else {
        // Non-Solder: download the zip file
        setStatus(tr("Downloading modpack:\n%1").arg(m_downloadUrl.toString()));

        const QString path = m_downloadUrl.host() + '/' + m_downloadUrl.path();
        auto entry = APPLICATION->metacache()->resolveEntry("general", path);
        entry->setStale(true);

        m_filesNetJob.reset(new NetJob(tr("Modpack download"), APPLICATION->network()));
        m_filesNetJob->addNetAction(Net::ApiDownload::makeCached(m_downloadUrl, entry));

        QString archivePath = entry->getFullPath();
        bool downloadSucceeded = false;

        connect(m_filesNetJob.get(), &NetJob::succeeded, this, [&downloadSucceeded, &loop]() {
            downloadSucceeded = true;
            loop.quit();
        });

        connect(m_filesNetJob.get(), &NetJob::failed, this, [this, &loop](QString reason) {
            m_abortable = false;
            m_filesNetJob.reset();
            setError(reason);
            loop.quit();
        });

        connect(m_filesNetJob.get(), &NetJob::aborted, this, [this, &loop]() {
            m_filesNetJob.reset();
            m_abort = true;
            loop.quit();
        });

        connect(m_filesNetJob.get(), &NetJob::progress, this, [this](qint64 current, qint64 total) {
            m_abortable = true;
            setProgress(current / 2, total);
        });
        connect(m_filesNetJob.get(), &NetJob::stepProgress, this, &InstanceCreationTask::propagateStepProgress);

        m_filesNetJob->start();
        loop.exec();

        m_abortable = false;
        m_filesNetJob.reset();

        if (!downloadSucceeded) {
            // Error or abort already handled
            return false;
        }

        // Extract synchronously
        setStatus(tr("Extracting modpack..."));
        QString extractDir = FS::PathCombine(m_stagingPath, "minecraft");
        FS::ensureFolderPathExists(extractDir);

        MMCZip::ArchiveReader packZip(archivePath);
        auto result = MMCZip::extractSubDir(&packZip, QString(""), extractDir);
        if (!result.has_value()) {
            setError(tr("Failed to extract modpack"));
            return false;
        }

        if (!processInstance()) {
            return false;
        }
    }

    if (m_abort)
        return false;

    return getError().isEmpty();
}

bool InstanceCreationTask::extractMods()
{
    setStatus(tr("Extracting modpack..."));

    int i = 0;
    QString extractDir = FS::PathCombine(m_stagingPath, "minecraft");
    FS::ensureFolderPathExists(extractDir);

    while (m_modCount > i) {
        auto path = FS::PathCombine(m_outputDir.path(), QString("%1").arg(i));
        auto result = MMCZip::extractDir(path, extractDir);
        if (!result.has_value()) {
            setError(tr("Failed to extract modpack"));
            return false;
        }
        i++;
    }

    return processInstance();
}

bool InstanceCreationTask::processInstance()
{
    QDir extractDir(m_stagingPath);

    qDebug() << "Fixing permissions for extracted pack files...";
    QDirIterator it(extractDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto filepath = it.next();
        QFileInfo file(filepath);
        auto permissions = QFile::permissions(filepath);
        auto origPermissions = permissions;
        if (file.isDir()) {
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser;
        } else {
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;
        }
        if (origPermissions != permissions) {
            if (!QFile::setPermissions(filepath, permissions)) {
                logWarning(tr("Could not fix permissions for %1").arg(filepath));
            } else {
                qDebug() << "Fixed" << filepath;
            }
        }
    }

    // Use TechnicPackProcessor to set up the instance
    // Note: TechnicPackProcessor::run() is synchronous - it emits signals before returning
    bool success = false;
    auto packProcessor = makeShared<TechnicPackProcessor>();

    connect(packProcessor.get(), &TechnicPackProcessor::succeeded, this, [this, &success]() {
        // After processing, set the managed pack info
        QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
        auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
        instanceSettings->registerSetting("ManagedPack", false);
        instanceSettings->registerSetting("ManagedPackType", QString());
        instanceSettings->registerSetting("ManagedPackID", QString());
        instanceSettings->registerSetting("ManagedPackName", QString());
        instanceSettings->registerSetting("ManagedPackVersionID", QString());
        instanceSettings->registerSetting("ManagedPackVersionName", QString());

        // Store Technic-specific settings
        instanceSettings->registerSetting("TechnicSolderUrl", QString());
        instanceSettings->registerSetting("TechnicIsSolder", false);

        instanceSettings->set("ManagedPack", true);
        instanceSettings->set("ManagedPackType", "technic");
        instanceSettings->set("ManagedPackID", m_slug);
        instanceSettings->set("ManagedPackName", name());
        instanceSettings->set("ManagedPackVersionID", m_version);
        instanceSettings->set("ManagedPackVersionName", m_version);

        instanceSettings->set("TechnicIsSolder", m_isSolder);
        if (m_isSolder) {
            instanceSettings->set("TechnicSolderUrl", m_solderUrl);
        }

        // If updating, copy managed pack info to the original instance
        if (m_instance.has_value()) {
            auto inst = m_instance.value();
            inst->settings()->set("ManagedPack", true);
            inst->settings()->set("ManagedPackType", "technic");
            inst->settings()->set("ManagedPackID", m_slug);
            inst->settings()->set("ManagedPackName", name());
            inst->settings()->set("ManagedPackVersionID", m_version);
            inst->settings()->set("ManagedPackVersionName", m_version);
            inst->settings()->set("TechnicIsSolder", m_isSolder);
            if (m_isSolder) {
                inst->settings()->set("TechnicSolderUrl", m_solderUrl);
            }
        }

        success = true;
    });

    connect(packProcessor.get(), &TechnicPackProcessor::failed, this, [this](QString reason) { setError(reason); });

    packProcessor->run(APPLICATION->settings(), name(), m_instIcon, m_stagingPath, m_minecraftVersion, m_isSolder);

    return success;
}

}  // namespace Technic
