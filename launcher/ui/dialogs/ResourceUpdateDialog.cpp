#include "ResourceUpdateDialog.h"
#include "Application.h"
#include "ChooseProviderDialog.h"
#include "CustomMessageBox.h"
#include "ProgressDialog.h"
#include "ScrollMessageBox.h"
#include "StringUtils.h"
#include "minecraft/mod/tasks/GetModDependenciesTask.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "tasks/SequentialTask.h"
#include "ui_ReviewMessageBox.h"

#include "Markdown.h"

#include "tasks/ConcurrentTask.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "modplatform/EnsureMetadataTask.h"
#include "modplatform/flame/FlameCheckUpdate.h"
#include "modplatform/modrinth/ModrinthCheckUpdate.h"

#include <QClipboard>
#include <QShortcut>
#include <QTextBrowser>
#include <QTreeWidgetItem>

#include <optional>

namespace {
std::vector<Version> mcVersions(BaseInstance* inst)
{
    return { static_cast<MinecraftInstance*>(inst)->getPackProfile()->getComponent("net.minecraft")->getVersion() };
}
}  // namespace

ResourceUpdateDialog::ResourceUpdateDialog(QWidget* parent,
                                           BaseInstance* instance,
                                           ResourceFolderModel* resourceModel,
                                           QList<Resource*>& searchFor,
                                           bool includeDeps,
                                           QList<ModPlatform::ModLoaderType> loadersList)
    : ReviewMessageBox(parent, tr("Confirm resources to update"), "")
    , m_parent(parent)
    , m_resourceModel(resourceModel)
    , m_candidates(searchFor)
    , m_secondTryMetadata(new ConcurrentTask("Second Metadata Search", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt()))
    , m_instance(instance)
    , m_includeDeps(includeDeps)
    , m_loadersList(std::move(loadersList))
{
    ReviewMessageBox::setGeometry(0, 0, 800, 600);

    ui->explainLabel->setText(tr("You're about to update the following resources:"));
    ui->onlyCheckedLabel->setText(tr("Only resources with a check will be updated!"));
}

void ResourceUpdateDialog::checkCandidates()
{
    // Ensure mods have valid metadata
    auto wentWell = ensureMetadata();
    if (!wentWell) {
        m_aborted = true;
        return;
    }

    // Report failed metadata generation
    if (!m_failedMetadata.empty()) {
        QString text;
        for (const auto& failed : m_failedMetadata) {
            const auto& mod = std::get<0>(failed);
            const auto& reason = std::get<1>(failed);
            text += tr("Mod name: %1<br>File name: %2<br>Reason: %3<br><br>").arg(mod->name(), mod->fileinfo().fileName(), reason);
        }

        ScrollMessageBox messageDialog(m_parent, tr("Metadata generation failed"),
                                       tr("Could not generate metadata for the following resources:<br>"
                                          "Do you wish to proceed without those resources?"),
                                       text);
        messageDialog.setModal(true);
        if (messageDialog.exec() == QDialog::Rejected) {
            m_aborted = true;
            QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            return;
        }
    }

    auto versions = mcVersions(m_instance);

    SequentialTask checkTask(tr("Checking for updates"));

    if (!m_modrinthToUpdate.empty()) {
        m_modrinthCheckTask.reset(new ModrinthCheckUpdate(m_modrinthToUpdate, versions, m_loadersList, m_resourceModel));
        connect(m_modrinthCheckTask.get(), &CheckUpdateTask::checkFailed, this,
                [this](Resource* resource, const QString& reason, const QUrl& recoverUrl) {
                    m_failedCheckUpdate.append({ resource, reason, recoverUrl });
                });
        checkTask.addTask(m_modrinthCheckTask);
    }

    if (!m_flameToUpdate.empty()) {
        m_flameCheckTask.reset(new FlameCheckUpdate(m_flameToUpdate, versions, m_loadersList, m_resourceModel));
        connect(m_flameCheckTask.get(), &CheckUpdateTask::checkFailed, this,
                [this](Resource* resource, const QString& reason, const QUrl& recoverUrl) {
                    m_failedCheckUpdate.append({ resource, reason, recoverUrl });
                });
        checkTask.addTask(m_flameCheckTask);
    }

    connect(&checkTask, &Task::failed, this,
            [this](const QString& reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec(); });

    connect(&checkTask, &Task::succeeded, this, [this, &checkTask]() {
        QStringList warnings = checkTask.warnings();
        if (warnings.count()) {
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->exec();
        }
    });

    // Check for updates
    ProgressDialog progressDialog(m_parent);
    progressDialog.setSkipButton(true, tr("Abort"));
    progressDialog.setWindowTitle(tr("Checking for updates..."));
    auto ret = progressDialog.execWithTask(&checkTask);

    // If the dialog was skipped / some download error happened
    if (ret == QDialog::DialogCode::Rejected) {
        m_aborted = true;
        QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
        return;
    }

    QList<std::shared_ptr<GetModDependenciesTask::PackDependency>> selectedVers;

    // Add found updates for Modrinth
    if (m_modrinthCheckTask) {
        auto modrinthUpdates = m_modrinthCheckTask->getUpdates();
        for (auto& updatable : modrinthUpdates) {
            qDebug() << QString("Mod %1 has an update available!").arg(updatable.name);

            appendResource(updatable);
            m_tasks.insert(updatable.name, updatable.download);
        }
        selectedVers.append(m_modrinthCheckTask->getDependencies());
    }

    // Add found updated for Flame
    if (m_flameCheckTask) {
        auto flameUpdates = m_flameCheckTask->getUpdates();
        for (auto& updatable : flameUpdates) {
            qDebug() << QString("Mod %1 has an update available!").arg(updatable.name);

            appendResource(updatable);
            m_tasks.insert(updatable.name, updatable.download);
        }
        selectedVers.append(m_flameCheckTask->getDependencies());
    }

    // Report failed update checking
    if (!m_failedCheckUpdate.empty()) {
        QString text;
        for (const auto& failed : m_failedCheckUpdate) {
            const auto& mod = std::get<0>(failed);
            const auto& reason = std::get<1>(failed);
            const auto& recoverUrl = std::get<2>(failed);

            qDebug() << mod->name() << "failed to check for updates!";

            text += tr("Mod name: %1").arg(mod->name()) + "<br>";
            if (!reason.isEmpty()) {
                text += tr("Reason: %1").arg(reason) + "<br>";
            }
            if (!recoverUrl.isEmpty()) {
                //: %1 is the link to download it manually
                text += tr("Possible solution: Getting the latest version manually:<br>%1<br>")
                            .arg(QString("<a href='%1'>%1</a>").arg(recoverUrl.toString()));
            }
            text += "<br>";
        }

        ScrollMessageBox messageDialog(m_parent, tr("Failed to check for updates"),
                                       tr("Could not check or get the following resources for updates:<br>"
                                          "Do you wish to proceed without those resources?"),
                                       text, "Disable unavailable mods");
        messageDialog.setModal(true);
        if (messageDialog.exec() == QDialog::Rejected) {
            m_aborted = true;
            QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            return;
        }

        // Disable unavailable mods
        if (messageDialog.isOptionChecked()) {
            for (const auto& failed : m_failedCheckUpdate) {
                const auto& mod = std::get<0>(failed);
                mod->enable(EnableAction::DISABLE);
            }
        }
    }

    if (m_includeDeps && !APPLICATION->settings()->get("ModDependenciesDisabled").toBool()) {  // dependencies
        auto* modModel = dynamic_cast<ModFolderModel*>(m_resourceModel);

        if (modModel != nullptr) {
            auto depTask = makeShared<GetModDependenciesTask>(m_instance, modModel, selectedVers);

            connect(depTask.get(), &Task::failed, this, [this](const QString& reason) {
                CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec();
            });
            auto weak = depTask.toWeakRef();
            connect(depTask.get(), &Task::succeeded, this, [this, weak]() {
                QStringList warnings;
                if (auto depTask = weak.lock()) {
                    warnings = depTask->warnings();
                }
                if (warnings.count()) {
                    CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->exec();
                }
            });

            ProgressDialog progressDialogDeps(m_parent);
            progressDialogDeps.setSkipButton(true, tr("Abort"));
            progressDialogDeps.setWindowTitle(tr("Checking for dependencies..."));
            auto dret = progressDialogDeps.execWithTask(depTask.get());

            // If the dialog was skipped / some download error happened
            if (dret == QDialog::DialogCode::Rejected) {
                m_aborted = true;
                QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
                return;
            }
            static FlameAPI s_api;

            auto dependencyExtraInfo = depTask->getExtraInfo();

            for (const auto& dep : depTask->getDependecies()) {
                auto changelog = dep->version.changelog;
                if (dep->pack->provider == ModPlatform::ResourceProvider::FLAME) {
                    changelog = s_api.getModFileChangelog(dep->version.addonId.toInt(), dep->version.fileId.toInt());
                }
                auto downloadTask = makeShared<ResourceDownloadTask>(dep->pack, dep->version, m_resourceModel, true, "dependency");
                auto extraInfo = dependencyExtraInfo.value(dep->version.addonId.toString());
                CheckUpdateTask::Update updatable = {
                    dep->pack->name, dep->version.hash,   tr("Not installed"), dep->version.version,      dep->version.version_type,
                    changelog,       dep->pack->provider, downloadTask,        !extraInfo.maybe_installed
                };

                appendResource(updatable, extraInfo.required_by);
                m_tasks.insert(updatable.name, updatable.download);
            }
        }
    }

    // If there's no resource to be updated
    if (ui->modTreeWidget->topLevelItemCount() == 0) {
        m_noUpdates = true;
    } else {
        // FIXME: Find a more efficient way of doing this!

        // Sort major items in alphabetical order (also sorts the children unfortunately)
        ui->modTreeWidget->sortItems(0, Qt::SortOrder::AscendingOrder);

        // Re-sort the children
        auto* item = ui->modTreeWidget->topLevelItem(0);
        for (int i = 1; item != nullptr; ++i) {
            item->sortChildren(0, Qt::SortOrder::DescendingOrder);
            item = ui->modTreeWidget->topLevelItem(i);
        }
    }

    if (m_aborted || m_noUpdates) {
        QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
    }
}

// Part 1: Ensure we have a valid metadata
auto ResourceUpdateDialog::ensureMetadata() -> bool
{
    auto indexDir2 = indexDir();

    SequentialTask seq(tr("Looking for metadata"));

    // A better use of data structures here could remove the need for this QHash
    QHash<QString, bool> shouldTryOthers;
    QList<Resource*> modrinthTmp;
    QList<Resource*> flameTmp;

    bool confirmRest = false;
    bool tryOthersRest = false;
    bool skipRest = false;
    ModPlatform::ResourceProvider providerRest = ModPlatform::ResourceProvider::MODRINTH;

    // adds resource to list based on provider
    auto addToTmp = [&modrinthTmp, &flameTmp](Resource* resource, ModPlatform::ResourceProvider p) {
        switch (p) {
            case ModPlatform::ResourceProvider::MODRINTH:
                modrinthTmp.push_back(resource);
                break;
            case ModPlatform::ResourceProvider::FLAME:
                flameTmp.push_back(resource);
                break;
        }
    };

    // ask the user on what provider to seach for the mod first
    for (auto* candidate : m_candidates) {
        if (candidate->status() != ResourceStatus::NoMetadata) {
            onMetadataEnsured(candidate);
            continue;
        }

        if (skipRest) {
            continue;
        }

        if (candidate->type() == ResourceType::FOLDER) {
            continue;
        }

        if (confirmRest) {
            addToTmp(candidate, providerRest);
            shouldTryOthers.insert(candidate->internalId(), tryOthersRest);
            continue;
        }

        ChooseProviderDialog chooser(this);
        chooser.setDescription(tr("The resource '%1' does not have a metadata yet. We need to generate it in order to track relevant "
                                  "information on how to update this mod. "
                                  "To do this, please select a mod provider which we can use to check for updates for this mod.")
                                   .arg(candidate->name()));
        auto confirmed = chooser.exec() == QDialog::DialogCode::Accepted;

        auto response = chooser.getResponse();

        if (response.skip_all) {
            skipRest = true;
        }
        if (response.confirm_all) {
            confirmRest = true;
            providerRest = response.chosen;
            tryOthersRest = response.try_others;
        }

        shouldTryOthers.insert(candidate->internalId(), response.try_others);

        if (confirmed) {
            addToTmp(candidate, response.chosen);
        }
    }

    // prepare task for the modrinth mods
    if (!modrinthTmp.empty()) {
        auto modrinthTask = makeShared<EnsureMetadataTask>(modrinthTmp, indexDir2, ModPlatform::ResourceProvider::MODRINTH);
        connect(modrinthTask.get(), &EnsureMetadataTask::metadataReady, [this](Resource* candidate) { onMetadataEnsured(candidate); });
        connect(modrinthTask.get(), &EnsureMetadataTask::metadataFailed, [this, &shouldTryOthers](Resource* candidate) {
            onMetadataFailed(candidate, shouldTryOthers.find(candidate->internalId()).value(), ModPlatform::ResourceProvider::MODRINTH);
        });
        connect(modrinthTask.get(), &EnsureMetadataTask::failed,
                [this](const QString& reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec(); });

        if (modrinthTask->getHashingTask()) {
            seq.addTask(modrinthTask->getHashingTask());
        }

        seq.addTask(modrinthTask);
    }

    // prepare task for the flame mods
    if (!flameTmp.empty()) {
        auto flameTask = makeShared<EnsureMetadataTask>(flameTmp, indexDir2, ModPlatform::ResourceProvider::FLAME);
        connect(flameTask.get(), &EnsureMetadataTask::metadataReady, [this](Resource* candidate) { onMetadataEnsured(candidate); });
        connect(flameTask.get(), &EnsureMetadataTask::metadataFailed, [this, &shouldTryOthers](Resource* candidate) {
            onMetadataFailed(candidate, shouldTryOthers.find(candidate->internalId()).value(), ModPlatform::ResourceProvider::FLAME);
        });
        connect(flameTask.get(), &EnsureMetadataTask::failed,
                [this](const QString& reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec(); });

        if (flameTask->getHashingTask()) {
            seq.addTask(flameTask->getHashingTask());
        }

        seq.addTask(flameTask);
    }

    seq.addTask(m_secondTryMetadata);

    // execute all the tasks
    ProgressDialog checkingDialog(m_parent);
    checkingDialog.setSkipButton(true, tr("Abort"));
    checkingDialog.setWindowTitle(tr("Generating metadata..."));
    auto retMetadata = checkingDialog.execWithTask(&seq);

    return (retMetadata != QDialog::DialogCode::Rejected);
}

void ResourceUpdateDialog::onMetadataEnsured(Resource* resource)
{
    // When the mod is a folder, for instance
    if (!resource->metadata()) {
        return;
    }

    switch (resource->metadata()->provider) {
        case ModPlatform::ResourceProvider::MODRINTH:
            m_modrinthToUpdate.push_back(resource);
            break;
        case ModPlatform::ResourceProvider::FLAME:
            m_flameToUpdate.push_back(resource);
            break;
    }
}

ModPlatform::ResourceProvider next(ModPlatform::ResourceProvider p)
{
    switch (p) {
        case ModPlatform::ResourceProvider::MODRINTH:
            return ModPlatform::ResourceProvider::FLAME;
        case ModPlatform::ResourceProvider::FLAME:
            return ModPlatform::ResourceProvider::MODRINTH;
    }

    return ModPlatform::ResourceProvider::FLAME;
}

void ResourceUpdateDialog::onMetadataFailed(Resource* resource, bool tryOthers, ModPlatform::ResourceProvider firstChoice)
{
    if (tryOthers) {
        auto indexDir2 = indexDir();

        auto task = makeShared<EnsureMetadataTask>(resource, indexDir2, next(firstChoice));
        connect(task.get(), &EnsureMetadataTask::metadataReady, [this](Resource* candidate) { onMetadataEnsured(candidate); });
        connect(task.get(), &EnsureMetadataTask::metadataFailed, [this](Resource* candidate) { onMetadataFailed(candidate, false); });
        connect(task.get(), &EnsureMetadataTask::failed,
                [this](const QString& reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec(); });
        if (task->getHashingTask()) {
            auto seq = makeShared<SequentialTask>();
            seq->addTask(task->getHashingTask());
            seq->addTask(task);
            m_secondTryMetadata->addTask(seq);
        } else {
            m_secondTryMetadata->addTask(task);
        }
    } else {
        QString reason{ tr("Couldn't find a valid version on the selected mod provider(s)") };

        m_failedMetadata.append({ resource, reason });
    }
}

void ResourceUpdateDialog::appendResource(const CheckUpdateTask::Update& info, QStringList requiredBy)
{
    auto* itemTop = new QTreeWidgetItem(ui->modTreeWidget);
    itemTop->setCheckState(0, info.enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    if (!info.enabled) {
        itemTop->setToolTip(0, tr("Mod was disabled as it may be already installed."));
    }
    itemTop->setText(0, info.name);
    itemTop->setExpanded(true);

    auto* providerItem = new QTreeWidgetItem(itemTop);
    QString providerName = ModPlatform::ProviderCapabilities::readableName(info.provider);
    providerItem->setText(0, tr("Provider: %1").arg(providerName));
    providerItem->setData(0, Qt::UserRole, providerName);

    auto* oldVersionItem = new QTreeWidgetItem(itemTop);
    oldVersionItem->setText(0, tr("Old version: %1").arg(info.oldVersion));
    oldVersionItem->setData(0, Qt::UserRole, info.oldVersion);

    auto* newVersionItem = new QTreeWidgetItem(itemTop);
    newVersionItem->setText(0, tr("New version: %1").arg(info.newVersion));
    newVersionItem->setData(0, Qt::UserRole, info.newVersion);

    if (info.newVersionType.has_value()) {
        auto* newVersionTypeItem = new QTreeWidgetItem(itemTop);
        newVersionTypeItem->setText(0, tr("New Version Type: %1").arg(info.newVersionType.value().toString()));
        newVersionTypeItem->setData(0, Qt::UserRole, info.newVersionType.value().toString());
    }

    if (!requiredBy.isEmpty()) {
        auto* requiredByItem = new QTreeWidgetItem(itemTop);
        if (requiredBy.length() == 1) {
            requiredByItem->setText(0, tr("Required by: %1").arg(requiredBy.back()));
            requiredByItem->setData(0, Qt::UserRole, requiredBy.back());
        } else {
            requiredByItem->setText(0, tr("Required by:"));
            for (const auto& req : requiredBy) {
                auto* reqItem = new QTreeWidgetItem(requiredByItem);
                reqItem->setText(0, req);
            }
        }

        ui->toggleDepsButton->show();
        m_deps << itemTop;
    }

    auto* changelogItem = new QTreeWidgetItem(itemTop);
    changelogItem->setText(0, tr("Changelog of the latest version"));

    auto* changelog = new QTreeWidgetItem(changelogItem);
    auto* changelogArea = new QTextBrowser();

    QString text = info.changelog;
    changelog->setData(0, Qt::UserRole, text);
    if (info.provider == ModPlatform::ResourceProvider::MODRINTH) {
        text = markdownToHTML(info.changelog.toUtf8());
    }

    changelogArea->setHtml(StringUtils::htmlListPatch(text));
    changelogArea->setOpenExternalLinks(true);
    changelogArea->setLineWrapMode(QTextBrowser::LineWrapMode::WidgetWidth);
    changelogArea->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);

    ui->modTreeWidget->setItemWidget(changelog, 0, changelogArea);

    ui->modTreeWidget->addTopLevelItem(itemTop);
}

auto ResourceUpdateDialog::getTasks() -> const QList<ResourceDownloadTask::Ptr>
{
    QList<ResourceDownloadTask::Ptr> list;

    auto* item = ui->modTreeWidget->topLevelItem(0);

    for (int i = 1; item != nullptr; ++i) {
        if (item->checkState(0) == Qt::CheckState::Checked) {
            list.push_back(m_tasks.find(item->text(0)).value());
        }

        item = ui->modTreeWidget->topLevelItem(i);
    }

    return list;
}
