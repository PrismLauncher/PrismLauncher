// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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

#include "UrlImportHandler.h"

#include <QUrl>
#include <QUrlQuery>

#include "Application.h"
#include "BuildConfig.h"
#include "InstanceList.h"
#include "minecraft/WorldList.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourcePackFolderModel.h"
#include "minecraft/mod/ShaderPackFolderModel.h"
#include "minecraft/mod/TexturePackFolderModel.h"
#include "minecraft/mod/tasks/LocalResourceParse.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "net/ApiRequest.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ImportResourceDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

void UrlHandler::process(const QList<QUrl>& urls)
{
    // NOTE: This loop only processes one dropped file!
    for (auto url : urls) {
        if (url.isEmpty() || url.toString().trimmed().isEmpty()) {
            continue;
        }

        qDebug() << "Processing" << url;

        // The isLocalFile() check below doesn't work as intended without an explicit scheme.
        if (url.scheme().isEmpty()) {
            url.setScheme("file");
        }

        if (url.isLocalFile()) {
            processFile(url);
            continue;
        }
        // download the remote resource and identify
        const bool isExternalURLImport = (url.host().toLower() == "import") || (url.path().startsWith("/import", Qt::CaseInsensitive));

        if (url.scheme() == "modrinth") {
            handleModrinth(url);
        } else if (url.scheme() == "curseforge" || (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME && url.host() == "install")) {
            handleCurseforge(url);
        } else if (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME && !isExternalURLImport) {
            handleOauth(url);
        } else if ((url.scheme() == "prismlauncher" || url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME) && isExternalURLImport) {
            handlePrism(url);
        } else {
            downloadFile(url);
        }
    }
}

void UrlHandler::processFile(const QUrl& url,
                             const ModPlatform::IndexedVersion& version,
                             const QMap<QString, QString>& extraInfo,
                             ModPlatform::ResourceProvider provider)
{
    auto localFileName = QDir::toNativeSeparators(url.toLocalFile());
    QFileInfo localFileInfo(localFileName);

    if (localFileName.isEmpty() || !localFileInfo.exists()) {
        qDebug() << "Ignoring invalid path" << localFileName;
        return;
    }

    auto type = ResourceUtils::identify(localFileInfo);

    if (!ModPlatform::ResourceTypeUtils::g_VALID_RESOURCES.contains(type)) {  // probably instance/modpack
        emit addInstance(localFileName, extraInfo);
        return;
    }

    if (APPLICATION->instances()->count() <= 0) {
        CustomMessageBox::selectable(m_parent, tr("No instance!"),
                                     tr("No instance available to add the resource to.\nPlease create a new instance before "
                                        "attempting to install this resource again."),
                                     QMessageBox::Critical)
            ->show();
        return;
    }
    ImportResourceDialog dlg(localFileName, type, m_parent);

    dlg.sortBy(version.mcVersion, version.loaders);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    qDebug() << "Adding resource" << localFileName << "to" << dlg.selectedInstanceKey;

    auto* minecraftInst = APPLICATION->instances()->getInstanceById(dlg.selectedInstanceKey);
    if (!minecraftInst) {
        CustomMessageBox::selectable(m_parent, tr("No instance!"),
                                     tr("The selected instance no longer exists.\nPlease try importing this resource again."),
                                     QMessageBox::Critical)
            ->show();
        return;
    }

    switch (type) {
        case ModPlatform::ResourceType::ResourcePack:
            minecraftInst->resourcePackList()->installResourceWithMeta(localFileName, version, provider);
            break;
        case ModPlatform::ResourceType::TexturePack:
            minecraftInst->texturePackList()->installResourceWithMeta(localFileName, version, provider);
            break;
        case ModPlatform::ResourceType::DataPack:
            if (auto* dataPackList = minecraftInst->dataPackList()) {
                dataPackList->installResourceWithMeta(localFileName, version, provider);
            } else {
                qWarning() << "Data packs are disabled for this instance. Ignoring" << localFileName;
            }
            break;
        case ModPlatform::ResourceType::Mod:
            minecraftInst->loaderModList()->installResourceWithMeta(localFileName, version, provider);
            break;
        case ModPlatform::ResourceType::ShaderPack:
            minecraftInst->shaderPackList()->installResourceWithMeta(localFileName, version, provider);
            break;
        case ModPlatform::ResourceType::World:
            minecraftInst->worldList()->installWorld(localFileInfo);
            break;
        case ModPlatform::ResourceType::Unknown:
        default:
            qDebug() << "Can't Identify" << localFileName << "Ignoring it.";
            break;
    }
}

void UrlHandler::handleOauth(const QUrl& url)
{
    QVariantMap receivedData;
    const QUrlQuery query(url.query());
    const auto items = query.queryItems();
    for (auto it = items.begin(), end = items.end(); it != end; ++it) {
        receivedData.insert(it->first, it->second);
    }
    emit APPLICATION->oauthReplyRecieved(receivedData);
}

void UrlHandler::downloadFile(const QUrl& url,
                              const ModPlatform::IndexedVersion& version,
                              const QMap<QString, QString>& extraInfo,
                              ModPlatform::ResourceProvider provider)
{
    if (!url.isValid()) {
        return;  // no valid url to download this resource
    }

    const QString path = url.host() + '/' + url.path();
    auto entry = APPLICATION->metacache()->resolveEntry("general", path);
    entry->setStale(true);
    auto dlJob = unique_qobject_ptr<NetJob>(new NetJob(tr("Modpack download"), APPLICATION->network()));
    dlJob->addNetAction(Net::ApiRequest::makeCached(url, entry));
    auto pathUrl = QUrl::fromLocalFile(entry->getFullPath());

    connect(dlJob.get(), &Task::failed, this,
            [this](const QString& reason) { CustomMessageBox::selectable(m_parent, tr("Error"), reason, QMessageBox::Critical)->show(); });
    connect(dlJob.get(), &Task::succeeded, this,
            [this, pathUrl, version, extraInfo, provider] { processFile(pathUrl, version, extraInfo, provider); });

    {  // drop stack
        ProgressDialog dlUrlDialod(m_parent);
        dlUrlDialod.setSkipButton(true, tr("Abort"));
        dlUrlDialod.execWithTask(dlJob.get());
    }
}

void UrlHandler::handleModrinth(const QUrl& url)
{
    const QString type = url.host();
    const QString id = url.path().mid(1);
    const QStringList supportedProtocols{ "modpack", "mod", "version" };
    if (id.isEmpty() || !supportedProtocols.contains(type)) {
        CustomMessageBox::selectable(m_parent, tr("Error"),
                                     tr("Unsupported Modrinth link.\n\nPrism Launcher currently only supports modrinth links such as "
                                        "modrinth://modpack/fabulously-optimized, modrinth://mod/fabric-api, modrinth://version/nr1znv5v."),
                                     QMessageBox::Critical)
            ->show();
        return;
    }
    if (type == "modpack") {
        emit addInstance(url.toString(), { { "pack_id", id } });
        return;
    }
    if (type == "mod") {
        if (APPLICATION->instances()->count() <= 0) {
            CustomMessageBox::selectable(m_parent, tr("No instance!"),
                                         tr("No instance available to add the resource to.\nPlease create a new instance before "
                                            "attempting to install this resource again."),
                                         QMessageBox::Critical)
                ->show();
            return;
        }
        auto [job, pack] = ModrinthAPI::get().getProjectTask(id);

        auto packPtr = std::make_shared<ModPlatform::IndexedPack>();
        *packPtr = *pack;
        ResourceAPI::VersionSearchArgs args{ .pack = packPtr };
        auto versionSpec = ModrinthAPI::get().getVersions(args);
        auto [vTask, versions] = Net::RPC::make<QList<ModPlatform::IndexedVersion>>(versionSpec);

        job->addNetAction(vTask);
        job->setMaxConcurrent(1);  // just to be sure is sync

        connect(job.get(), &Task::failed, this, [this](const QString& reason) {
            CustomMessageBox::selectable(m_parent, tr("Error"), reason, QMessageBox::Critical)->show();
        });

        {  // drop stack
            ProgressDialog dlUrlDialod(m_parent);
            dlUrlDialod.setSkipButton(true, tr("Abort"));
            dlUrlDialod.execWithTask(job.get());
        }

        QStringList mcVersion;
        ModPlatform::ModLoaderTypes loaders;
        for (const auto& v : *versions) {
            mcVersion.append(v.mcVersion);
            loaders |= v.loaders;
        }
        mcVersion.removeDuplicates();

        ImportResourceDialog dlg(pack->name, pack->resourceType, m_parent);

        dlg.sortBy(mcVersion, loaders);
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }

        auto* inst = APPLICATION->instances()->getInstanceById(dlg.selectedInstanceKey);
        if (!inst) {
            CustomMessageBox::selectable(m_parent, tr("No instance!"),
                                         tr("The selected instance no longer exists.\nPlease try importing this resource again."),
                                         QMessageBox::Critical)
                ->show();
            return;
        }
        ResourceDownload::ResourceDownloadDialog* rDlg = nullptr;
        switch (pack->resourceType) {
            case ModPlatform::ResourceType::ResourcePack:
                rDlg = ResourceDownload::ResourceDownloadDialog::createResourcePack(m_parent, inst->resourcePackList(), inst, true);
                break;
            case ModPlatform::ResourceType::TexturePack:
                rDlg = ResourceDownload::ResourceDownloadDialog::createTexturePack(m_parent, inst->texturePackList(), inst, true);
                break;
            case ModPlatform::ResourceType::DataPack:
                if (auto* dataPackList = inst->dataPackList()) {
                    rDlg = ResourceDownload::ResourceDownloadDialog::createDataPack(m_parent, dataPackList, inst, true);
                } else {
                    qWarning() << "Data packs are disabled for this instance. Ignoring" << pack->name;
                }
                break;
            case ModPlatform::ResourceType::Mod:
                rDlg = ResourceDownload::ResourceDownloadDialog::createMod(m_parent, inst->loaderModList(), inst, true);
                break;
            case ModPlatform::ResourceType::ShaderPack:
                rDlg = ResourceDownload::ResourceDownloadDialog::createShaderPack(m_parent, inst->shaderPackList(), inst, true);
                break;
            default:
                CustomMessageBox::selectable(m_parent, tr("Error"),
                                             tr("Unsupported Modrinth resource.\n\nPrism Launcher currently doesn't support %1.")
                                                 .arg(ModPlatform::ResourceTypeUtils::getName(pack->resourceType)),
                                             QMessageBox::Critical)
                    ->show();
                break;
        }
        if (rDlg) {
            rDlg->setResourcePack(*pack, ModPlatform::ResourceProvider::MODRINTH);
            if (rDlg->exec() != 0) {
                ConcurrentTask tasks("Download Data Packs", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
                connect(&tasks, &Task::failed, this, [this](const QString& reason) {
                    CustomMessageBox::selectable(m_parent, tr("Error"), reason, QMessageBox::Critical)->show();
                });
                connect(&tasks, &Task::succeeded, this, [this, &tasks]() {
                    QStringList warnings = tasks.warnings();
                    if (warnings.count()) {
                        CustomMessageBox::selectable(m_parent, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
                    }
                });

                for (auto& task : rDlg->getTasks()) {
                    tasks.addTask(task);
                }

                {
                    ProgressDialog loadDialog(m_parent);
                    loadDialog.setSkipButton(true, tr("Abort"));
                    loadDialog.execWithTask(&tasks);
                }
            }
            rDlg->deleteLater();
        }

        return;
    }
    if (type == "version") {
        auto [job, versionRes] = ModrinthAPI::get().getVersionTask({}, id);

        connect(job.get(), &Task::failed, this, [this](const QString& reason) {
            CustomMessageBox::selectable(m_parent, tr("Error"), reason, QMessageBox::Critical)->show();
        });
        connect(job.get(), &Task::succeeded, this, [versionRes, this] {
            // Have to use ensureString then use QUrl to get proper url encoding
            downloadFile(versionRes->downloadUrl, *versionRes,
                         { { "pack_id", versionRes->addonId.toString() }, { "pack_version_id", versionRes->version } },
                         ModPlatform::ResourceProvider::MODRINTH);
        });

        {  // drop stack
            ProgressDialog dlUrlDialod(m_parent);
            dlUrlDialod.setSkipButton(true, tr("Abort"));
            dlUrlDialod.execWithTask(job.get());
        }
    }
}

void UrlHandler::handleCurseforge(const QUrl& url)
{
    // need to find the download link for the modpack / resource
    // format of url curseforge://install?addonId=IDHERE&fileId=IDHERE
    // format of url binaryname://install?platform=curseforge&addonId=IDHERE&fileId=IDHERE
    QUrlQuery query(url);

    // check if this is a binaryname:// url
    if (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME) {
        // check this is an curseforge platform request
        if (query.queryItemValue("platform").toLower() != "curseforge") {
            qDebug() << "Invalid mod distribution platform:" << query.queryItemValue("platform");
            return;
        }
    }

    if (query.allQueryItemValues("addonId").isEmpty() || query.allQueryItemValues("fileId").isEmpty()) {
        qDebug() << "Invalid curseforge link:" << url;
        return;
    }

    auto addonId = query.allQueryItemValues("addonId")[0];
    auto fileId = query.allQueryItemValues("fileId")[0];

    auto [job, versionRes] = FlameAPI::get().getVersionTask(addonId, fileId);

    connect(job.get(), &Task::failed, this,
            [this](const QString& reason) { CustomMessageBox::selectable(m_parent, tr("Error"), reason, QMessageBox::Critical)->show(); });
    connect(job.get(), &Task::succeeded, this, [this, versionRes, addonId, fileId] {
        // Have to use ensureString then use QUrl to get proper url encoding
        auto dlUrl = QUrl(versionRes->downloadUrl);
        if (!dlUrl.isValid()) {
            CustomMessageBox::selectable(
                m_parent, tr("Error"),
                tr("The modpack, mod, or resource %1 is blocked for third-parties! Please download it manually.").arg(versionRes->fileName),
                QMessageBox::Critical)
                ->show();
            return;
        }
        downloadFile(dlUrl, *versionRes, { { "pack_id", addonId }, { "pack_version_id", fileId } }, ModPlatform::ResourceProvider::FLAME);
    });

    {  // drop stack
        ProgressDialog dlUrlDialod(m_parent);
        dlUrlDialod.setSkipButton(true, tr("Abort"));
        dlUrlDialod.execWithTask(job.get());
    }
}

void UrlHandler::handlePrism(const QUrl& url)
{
    // PrismLauncher URL protocol modpack import
    // works for any prism fork
    // preferred impoprocessFilert format: prismlauncher://import?url=ENCODED
    const auto host = url.host().toLower();
    const auto path = url.path();

    QString encodedTarget;

    {
        QUrlQuery query(url);
        const auto values = query.allQueryItemValues("url");
        if (!values.isEmpty()) {
            encodedTarget = values.first();
        }
    }

    // alternative import format: prismlauncher://import/ENCODED
    if (encodedTarget.isEmpty()) {
        QString p = path;

        if (p.startsWith("/import/", Qt::CaseInsensitive)) {
            p = p.mid(QString("/import/").size());
        } else if (host == "import" && p.startsWith("/")) {
            p = p.mid(1);
        }

        if (!p.isEmpty() && p != "/import") {
            encodedTarget = p;
        }
    }

    if (encodedTarget.isEmpty()) {
        CustomMessageBox::selectable(m_parent, tr("Error"), tr("Invalid import link: missing 'url' parameter."), QMessageBox::Critical)
            ->show();
        return;
    }

    const QString decodedStr = QUrl::fromPercentEncoding(encodedTarget.toUtf8()).trimmed();

    QUrl target = QUrl::fromUserInput(decodedStr);

    // Validate: only allow http(s)
    if (!target.isValid() || (target.scheme() != "https" && target.scheme() != "http")) {
        CustomMessageBox::selectable(m_parent, tr("Error"), tr("Invalid import link: URL must be http(s)."), QMessageBox::Critical)->show();
        return;
    }

    const auto res = QMessageBox::question(
        m_parent, tr("Install modpack"),
        tr("Do you want to download and import a modpack from:\n%1\n\nURL:\n%2").arg(target.host(), target.toString()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (res != QMessageBox::Yes) {
        return;
    }

    downloadFile(target);
}
