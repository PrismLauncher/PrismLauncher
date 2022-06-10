#include "ModUpdateDialog.h"
#include "ChooseProviderDialog.h"
#include "CustomMessageBox.h"
#include "ProgressDialog.h"
#include "ScrollMessageBox.h"
#include "ui_ReviewMessageBox.h"

#include "FileSystem.h"
#include "Json.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "modplatform/EnsureMetadataTask.h"
#include "modplatform/flame/FlameCheckUpdate.h"
#include "modplatform/modrinth/ModrinthCheckUpdate.h"

#include <HoeDown.h>

#include <QTextEdit>
#include <QTreeWidgetItem>

static ModPlatform::ProviderCapabilities ProviderCaps;

static std::list<Version> mcVersions(BaseInstance* inst)
{
    return { static_cast<MinecraftInstance*>(inst)->getPackProfile()->getComponent("net.minecraft")->getVersion() };
}

static ModAPI::ModLoaderTypes mcLoaders(BaseInstance* inst)
{
    return { static_cast<MinecraftInstance*>(inst)->getPackProfile()->getModLoaders() };
}

ModUpdateDialog::ModUpdateDialog(QWidget* parent,
                                 BaseInstance* instance,
                                 const std::shared_ptr<ModFolderModel>& mods,
                                 std::list<Mod>& search_for)
    : ReviewMessageBox(parent, tr("Confirm mods to update"), "")
    , m_parent(parent)
    , m_mod_model(mods)
    , m_candidates(search_for)
    , m_instance(instance)
{
    ReviewMessageBox::setGeometry(0, 0, 800, 600);

    ui->explainLabel->setText(tr("You're about to update the following mods:"));
    ui->onlyCheckedLabel->setText(tr("Only mods with a check will be updated!"));

    connect(&m_check_task, &Task::failed, this,
            [&](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec(); });

    connect(&m_check_task, &Task::succeeded, this, [&]() {
        QStringList warnings = m_check_task.warnings();
        if (warnings.count()) {
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->exec();
        }
    });
}

void ModUpdateDialog::checkCandidates()
{
    // Ensure mods have valid metadata
    auto went_well = ensureMetadata();
    if (!went_well) {
        m_aborted = true;
        return;
    }

    // Report failed metadata generation
    if (!m_failed_metadata.empty()) {
        QString text;
        for (const auto& mod : m_failed_metadata) {
            text += tr("Mod name: %1<br>File name: %2<br><br>").arg(mod.name(), mod.fileinfo().fileName());
        }

        ScrollMessageBox message_dialog(m_parent, tr("Metadata generation failed"),
                                                   tr("Could not generate metadata for the following mods:<br>"
                                                      "Do you wish to proceed without those mods?"),
                                                   text);
        message_dialog.setModal(true);
        if (message_dialog.exec() == QDialog::Rejected) {
            m_aborted = true;
            QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            return;
        }
    }

    auto versions = mcVersions(m_instance);
    auto loaders = mcLoaders(m_instance);

    if (!m_modrinth_to_update.empty()) {
        m_modrinth_check_task = new ModrinthCheckUpdate(m_modrinth_to_update, versions, loaders, m_mod_model);
        connect(m_modrinth_check_task, &CheckUpdateTask::checkFailed, this,
                [this](Mod mod, QString reason, QUrl recover_url) { m_failed_check_update.emplace_back(mod, reason, recover_url); });
        m_check_task.addTask(m_modrinth_check_task);
    }

    if (!m_flame_to_update.empty()) {
        m_flame_check_task = new FlameCheckUpdate(m_flame_to_update, versions, loaders, m_mod_model);
        connect(m_flame_check_task, &CheckUpdateTask::checkFailed, this,
                [this](Mod mod, QString reason, QUrl recover_url) { m_failed_check_update.emplace_back(mod, reason, recover_url); });
        m_check_task.addTask(m_flame_check_task);
    }

    // Check for updates
    ProgressDialog progress_dialog(m_parent);
    progress_dialog.setSkipButton(true, tr("Abort"));
    progress_dialog.setVisible(true);
    progress_dialog.setWindowTitle(tr("Checking for updates..."));
    auto ret = progress_dialog.execWithTask(&m_check_task);

    // If the dialog was skipped / some download error happened
    if (ret == QDialog::DialogCode::Rejected) {
        m_aborted = true;
        QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
        return;
    }

    // Add found updates for Modrinth
    if (m_modrinth_check_task) {
        auto modrinth_updates = m_modrinth_check_task->getUpdatable();
        for (auto& updatable : modrinth_updates) {
            qDebug() << QString("Mod %1 has an update available!").arg(updatable.name);

            appendMod(updatable);
            m_tasks.insert(updatable.name, updatable.download);
        }
    }

    // Add found updated for Flame
    if (m_flame_check_task) {
        auto flame_updates = m_flame_check_task->getUpdatable();
        for (auto& updatable : flame_updates) {
            qDebug() << QString("Mod %1 has an update available!").arg(updatable.name);

            appendMod(updatable);
            m_tasks.insert(updatable.name, updatable.download);
        }
    }

    // Report failed update checking
    if (!m_failed_check_update.empty()) {
        QString text;
        for (const auto& failed : m_failed_check_update) {
            const auto& mod = std::get<0>(failed);
            const auto& reason = std::get<1>(failed);
            const auto& recover_url = std::get<2>(failed);

            qDebug() << mod.name() << " failed to check for updates!";

            text += tr("Mod name: %1").arg(mod.name()) + "<br>";
            if (!reason.isEmpty())
                text += tr("Reason: %1").arg(reason) + "<br>";
            if (!recover_url.isEmpty())
                text += tr("Possible solution: ") + tr("Getting the latest version manually:") + "<br>" +
                        QString("<a href='%1'>").arg(recover_url.toString()) + recover_url.toString() + "</a><br>";
            text += "<br>";
        }

        ScrollMessageBox message_dialog(m_parent, tr("Failed to check for updates"),
                                                   tr("Could not check or get the following mods for updates:<br>"
                                                      "Do you wish to proceed without those mods?"),
                                                   text);
        message_dialog.setModal(true);
        if (message_dialog.exec() == QDialog::Rejected) {
            m_aborted = true;
            QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            return;
        }
    }

    // If there's no mod to be updated
    if (ui->modTreeWidget->topLevelItemCount() == 0)
        m_no_updates = true;

    if (m_aborted || m_no_updates)
        QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
}

// Part 1: Ensure we have a valid metadata
auto ModUpdateDialog::ensureMetadata() -> bool
{
    auto index_dir = indexDir();

    auto* seq = new SequentialTask(m_parent, tr("Looking for metadata"));

    bool confirm_rest = false;
    bool try_others_rest = false;
    bool skip_rest = false;
    ModPlatform::Provider provider_rest = ModPlatform::Provider::MODRINTH;

    for (auto& candidate : m_candidates) {
        if (candidate.status() != ModStatus::NoMetadata) {
            onMetadataEnsured(candidate);
            continue;
        }

        if (skip_rest)
            continue;

        if (confirm_rest) {
            auto* task = new EnsureMetadataTask(candidate, index_dir, try_others_rest, provider_rest);
            connect(task, &EnsureMetadataTask::metadataReady, [this, &candidate] { onMetadataEnsured(candidate); });
            connect(task, &EnsureMetadataTask::metadataFailed, [this, &candidate] { onMetadataFailed(candidate); });
            seq->addTask(task);
            continue;
        }

        ChooseProviderDialog chooser(this);
        chooser.setDescription(tr("This mod (%1) does not have a metadata yet. We need to create one in order to keep relevant "
                                  "information on how to update this "
                                  "mod. To do this, please select a mod provider from which we can search for updates for %1.")
                                   .arg(candidate.name()));
        auto confirmed = chooser.exec() == QDialog::DialogCode::Accepted;

        auto response = chooser.getResponse();

        if (response.skip_all)
            skip_rest = true;
        if (response.confirm_all) {
            confirm_rest = true;
            provider_rest = response.chosen;
            try_others_rest = response.try_others;
        }

        if (confirmed) {
            auto* task = new EnsureMetadataTask(candidate, index_dir, response.try_others, response.chosen);
            connect(task, &EnsureMetadataTask::metadataReady, [this, &candidate] { onMetadataEnsured(candidate); });
            connect(task, &EnsureMetadataTask::metadataFailed, [this, &candidate] { onMetadataFailed(candidate); });
            seq->addTask(task);
        }
    }

    ProgressDialog checking_dialog(m_parent);
    checking_dialog.setSkipButton(true, tr("Abort"));
    checking_dialog.setWindowTitle(tr("Generating metadata..."));
    auto ret_metadata = checking_dialog.execWithTask(seq);

    return (ret_metadata != QDialog::DialogCode::Rejected);
}

void ModUpdateDialog::onMetadataEnsured(Mod& mod)
{
    // When the mod is a folder, for instance
    if (!mod.metadata())
        return;

    switch (mod.metadata()->provider) {
        case ModPlatform::Provider::MODRINTH:
            m_modrinth_to_update.push_back(mod);
            break;
        case ModPlatform::Provider::FLAME:
            m_flame_to_update.push_back(mod);
            break;
    }
}

void ModUpdateDialog::onMetadataFailed(Mod& mod)
{
    m_failed_metadata.push_back(mod);
}

void ModUpdateDialog::appendMod(CheckUpdateTask::UpdatableMod const& info)
{
    auto item_top = new QTreeWidgetItem(ui->modTreeWidget);
    item_top->setCheckState(0, Qt::CheckState::Checked);
    item_top->setText(0, info.name);
    item_top->setExpanded(true);

    auto provider_item = new QTreeWidgetItem(item_top);
    provider_item->setText(0, tr("Provider: %1").arg(ProviderCaps.readableName(info.provider)));

    auto old_version_item = new QTreeWidgetItem(item_top);
    old_version_item->setText(0, tr("Old version: %1").arg(info.old_version.isEmpty() ? tr("Not installed") : info.old_version));

    auto new_version_item = new QTreeWidgetItem(item_top);
    new_version_item->setText(0, tr("New version: %1").arg(info.new_version));

    auto changelog_item = new QTreeWidgetItem(item_top);
    changelog_item->setText(0, tr("Changelog of the latest version"));

    auto changelog = new QTreeWidgetItem(changelog_item);

    auto changelog_area = new QTextEdit();
    HoeDown h;
    changelog_area->setText(h.process(info.changelog.toUtf8()));
    changelog_area->setReadOnly(true);
    if (info.changelog.size() < 250)  // heuristic
        changelog_area->setSizeAdjustPolicy(QTextEdit::SizeAdjustPolicy::AdjustToContents);

    ui->modTreeWidget->setItemWidget(changelog, 0, changelog_area);
    changelog_item->insertChildren(0, { changelog });

    item_top->insertChildren(0, { old_version_item });
    item_top->insertChildren(1, { new_version_item });
    item_top->insertChildren(2, { changelog_item });

    ui->modTreeWidget->addTopLevelItem(item_top);
}

auto ModUpdateDialog::getTasks() -> const std::list<ModDownloadTask*>
{
    std::list<ModDownloadTask*> list;

    auto* item = ui->modTreeWidget->topLevelItem(0);

    for (int i = 1; item != nullptr; ++i) {
        if (item->checkState(0) == Qt::CheckState::Checked) {
            list.push_back(m_tasks.find(item->text(0)).value());
        }

        item = ui->modTreeWidget->topLevelItem(i);
    }

    return list;
}
