#include "ModUpdateDialog.h"
#include "ChooseProviderDialog.h"
#include "CustomMessageBox.h"
#include "ProgressDialog.h"
#include "ScrollMessageBox.h"
#include "ui_ReviewMessageBox.h"

#include "FileSystem.h"
#include "Json.h"

#include "tasks/ConcurrentTask.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "modplatform/EnsureMetadataTask.h"
#include "modplatform/flame/FlameCheckUpdate.h"
#include "modplatform/modrinth/ModrinthCheckUpdate.h"

#include <HoeDown.h>
#include <QTextBrowser>
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
                                 const std::shared_ptr<ModFolderModel> mods,
                                 QList<Mod*>& search_for)
    : ReviewMessageBox(parent, tr("Confirm mods to update"), "")
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent(parent)
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_model(mods)
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_candidates(search_for)
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_second_try_metadata(new ConcurrentTask())
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance(instance)
{
    ReviewMessageBox::setGeometry(0, 0, 800, 600);

    ui->explainLabel->setText(tr("You're about to update the following mods:"));
    ui->onlyCheckedLabel->setText(tr("Only mods with a check will be updated!"));
}

void ModUpdateDialog::checkCandidates()
{
    // Ensure mods have valid metadata
    auto went_well = ensureMetadata();
    if (!went_well) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = true;
        return;
    }

    // Report failed metadata generation
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_metadata.empty()) {
        QString text;
        for (const auto& failed : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_metadata) {
            const auto& mod = std::get<0>(failed);
            const auto& reason = std::get<1>(failed);
            text += tr("Mod name: %1<br>File name: %2<br>Reason: %3<br><br>").arg(mod->name(), mod->fileinfo().fileName(), reason);
        }

        ScrollMessageBox message_dialog(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent, tr("Metadata generation failed"),
                                        tr("Could not generate metadata for the following mods:<br>"
                                           "Do you wish to proceed without those mods?"),
                                        text);
        message_dialog.setModal(true);
        if (message_dialog.exec() == QDialog::Rejected) {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = true;
            QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            return;
        }
    }

    auto versions = mcVersions(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance);
    auto loaders = mcLoaders(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance);

    SequentialTask check_task(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent, tr("Checking for updates"));

    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_to_update.empty()) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_check_task = new ModrinthCheckUpdate(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_to_update, versions, loaders, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_model);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_check_task, &CheckUpdateTask::checkFailed, this,
                [this](Mod* mod, QString reason, QUrl recover_url) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_check_update.append({mod, reason, recover_url}); });
        check_task.addTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_check_task);
    }

    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_to_update.empty()) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_check_task = new FlameCheckUpdate(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_to_update, versions, loaders, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_model);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_check_task, &CheckUpdateTask::checkFailed, this,
                [this](Mod* mod, QString reason, QUrl recover_url) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_check_update.append({mod, reason, recover_url}); });
        check_task.addTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_check_task);
    }

    connect(&check_task, &Task::failed, this,
            [&](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec(); });

    connect(&check_task, &Task::succeeded, this, [&]() {
        QStringList warnings = check_task.warnings();
        if (warnings.count()) {
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->exec();
        }
    });

    // Check for updates
    ProgressDialog progress_dialog(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent);
    progress_dialog.setSkipButton(true, tr("Abort"));
    progress_dialog.setWindowTitle(tr("Checking for updates..."));
    auto ret = progress_dialog.execWithTask(&check_task);

    // If the dialog was skipped / some download error happened
    if (ret == QDialog::DialogCode::Rejected) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = true;
        QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
        return;
    }

    // Add found updates for Modrinth
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_check_task) {
        auto modrinth_updates = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_check_task->getUpdatable();
        for (auto& updatable : modrinth_updates) {
            qDebug() << QString("Mod %1 has an update available!").arg(updatable.name);

            appendMod(updatable);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.insert(updatable.name, updatable.download);
        }
    }

    // Add found updated for Flame
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_check_task) {
        auto flame_updates = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_check_task->getUpdatable();
        for (auto& updatable : flame_updates) {
            qDebug() << QString("Mod %1 has an update available!").arg(updatable.name);

            appendMod(updatable);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.insert(updatable.name, updatable.download);
        }
    }

    // Report failed update checking
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_check_update.empty()) {
        QString text;
        for (const auto& failed : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_check_update) {
            const auto& mod = std::get<0>(failed);
            const auto& reason = std::get<1>(failed);
            const auto& recover_url = std::get<2>(failed);

            qDebug() << mod->name() << " failed to check for updates!";

            text += tr("Mod name: %1").arg(mod->name()) + "<br>";
            if (!reason.isEmpty())
                text += tr("Reason: %1").arg(reason) + "<br>";
            if (!recover_url.isEmpty())
                //: %1 is the link to download it manually
                text += tr("Possible solution: Getting the latest version manually:<br>%1<br>")
                    .arg(QString("<a href='%1'>%1</a>").arg(recover_url.toString()));
            text += "<br>";
        }

        ScrollMessageBox message_dialog(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent, tr("Failed to check for updates"),
                                        tr("Could not check or get the following mods for updates:<br>"
                                           "Do you wish to proceed without those mods?"),
                                        text);
        message_dialog.setModal(true);
        if (message_dialog.exec() == QDialog::Rejected) {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = true;
            QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            return;
        }
    }

    // If there's no mod to be updated
    if (ui->modTreeWidget->topLevelItemCount() == 0) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_no_updates = true;
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

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_no_updates)
        QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
}

// Part 1: Ensure we have a valid metadata
auto ModUpdateDialog::ensureMetadata() -> bool
{
    auto index_dir = indexDir();

    SequentialTask seq(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent, tr("Looking for metadata"));

    // A better use of data structures here could remove the need for this QHash
    QHash<QString, bool> should_try_others;
    QList<Mod*> modrinth_tmp;
    QList<Mod*> flame_tmp;

    bool confirhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rest = false;
    bool try_others_rest = false;
    bool skip_rest = false;
    ModPlatform::Provider provider_rest = ModPlatform::Provider::MODRINTH;

    auto addToTmp = [&](Mod* m, ModPlatform::Provider p) {
        switch (p) {
            case ModPlatform::Provider::MODRINTH:
                modrinth_tmp.push_back(m);
                break;
            case ModPlatform::Provider::FLAME:
                flame_tmp.push_back(m);
                break;
        }
    };

    for (auto candidate : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_candidates) {
        if (candidate->status() != ModStatus::NoMetadata) {
            onMetadataEnsured(candidate);
            continue;
        }

        if (skip_rest)
            continue;

        if (confirhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rest) {
            addToTmp(candidate, provider_rest);
            should_try_others.insert(candidate->internal_id(), try_others_rest);
            continue;
        }

        ChooseProviderDialog chooser(this);
        chooser.setDescription(tr("The mod '%1' does not have a metadata yet. We need to generate it in order to track relevant "
                                  "information on how to update this mod. "
                                  "To do this, please select a mod provider which we can use to check for updates for this mod.")
                                   .arg(candidate->name()));
        auto confirmed = chooser.exec() == QDialog::DialogCode::Accepted;

        auto response = chooser.getResponse();

        if (response.skip_all)
            skip_rest = true;
        if (response.confirhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_all) {
            confirhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rest = true;
            provider_rest = response.chosen;
            try_others_rest = response.try_others;
        }

        should_try_others.insert(candidate->internal_id(), response.try_others);

        if (confirmed)
            addToTmp(candidate, response.chosen);
    }

    if (!modrinth_tmp.empty()) {
        auto* modrinth_task = new EnsureMetadataTask(modrinth_tmp, index_dir, ModPlatform::Provider::MODRINTH);
        connect(modrinth_task, &EnsureMetadataTask::metadataReady, [this](Mod* candidate) { onMetadataEnsured(candidate); });
        connect(modrinth_task, &EnsureMetadataTask::metadataFailed, [this, &should_try_others](Mod* candidate) {
            onMetadataFailed(candidate, should_try_others.find(candidate->internal_id()).value(), ModPlatform::Provider::MODRINTH);
        });

        if (modrinth_task->getHashingTask())
            seq.addTask(modrinth_task->getHashingTask());

        seq.addTask(modrinth_task);
    }

    if (!flame_tmp.empty()) {
        auto* flame_task = new EnsureMetadataTask(flame_tmp, index_dir, ModPlatform::Provider::FLAME);
        connect(flame_task, &EnsureMetadataTask::metadataReady, [this](Mod* candidate) { onMetadataEnsured(candidate); });
        connect(flame_task, &EnsureMetadataTask::metadataFailed, [this, &should_try_others](Mod* candidate) {
            onMetadataFailed(candidate, should_try_others.find(candidate->internal_id()).value(), ModPlatform::Provider::FLAME);
        });

        if (flame_task->getHashingTask())
            seq.addTask(flame_task->getHashingTask());

        seq.addTask(flame_task);
    }

    seq.addTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_second_try_metadata);

    ProgressDialog checking_dialog(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent);
    checking_dialog.setSkipButton(true, tr("Abort"));
    checking_dialog.setWindowTitle(tr("Generating metadata..."));
    auto ret_metadata = checking_dialog.execWithTask(&seq);

    return (ret_metadata != QDialog::DialogCode::Rejected);
}

void ModUpdateDialog::onMetadataEnsured(Mod* mod)
{
    // When the mod is a folder, for instance
    if (!mod->metadata())
        return;

    switch (mod->metadata()->provider) {
        case ModPlatform::Provider::MODRINTH:
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_to_update.push_back(mod);
            break;
        case ModPlatform::Provider::FLAME:
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_to_update.push_back(mod);
            break;
    }
}

ModPlatform::Provider next(ModPlatform::Provider p)
{
    switch (p) {
        case ModPlatform::Provider::MODRINTH:
            return ModPlatform::Provider::FLAME;
        case ModPlatform::Provider::FLAME:
            return ModPlatform::Provider::MODRINTH;
    }

    return ModPlatform::Provider::FLAME;
}

void ModUpdateDialog::onMetadataFailed(Mod* mod, bool try_others, ModPlatform::Provider first_choice)
{
    if (try_others) {
        auto index_dir = indexDir();

        auto* task = new EnsureMetadataTask(mod, index_dir, next(first_choice));
        connect(task, &EnsureMetadataTask::metadataReady, [this](Mod* candidate) { onMetadataEnsured(candidate); });
        connect(task, &EnsureMetadataTask::metadataFailed, [this](Mod* candidate) { onMetadataFailed(candidate, false); });

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_second_try_metadata->addTask(task);
    } else {
        QString reason{ tr("Couldn't find a valid version on the selected mod provider(s)") };

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_metadata.append({mod, reason});
    }
}

void ModUpdateDialog::appendMod(CheckUpdateTask::UpdatableMod const& info)
{
    auto itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top = new QTreeWidgetItem(ui->modTreeWidget);
    itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top->setCheckState(0, Qt::CheckState::Checked);
    itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top->setText(0, info.name);
    itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top->setExpanded(true);

    auto provider_item = new QTreeWidgetItem(itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top);
    provider_item->setText(0, tr("Provider: %1").arg(ProviderCaps.readableName(info.provider)));

    auto old_version_item = new QTreeWidgetItem(itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top);
    old_version_item->setText(0, tr("Old version: %1").arg(info.old_version.isEmpty() ? tr("Not installed") : info.old_version));

    auto new_version_item = new QTreeWidgetItem(itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top);
    new_version_item->setText(0, tr("New version: %1").arg(info.new_version));

    auto changelog_item = new QTreeWidgetItem(itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top);
    changelog_item->setText(0, tr("Changelog of the latest version"));

    auto changelog = new QTreeWidgetItem(changelog_item);
    auto changelog_area = new QTextBrowser();

    QString text = info.changelog;
    switch (info.provider) {
        case ModPlatform::Provider::MODRINTH: {
            HoeDown h;
            // HoeDown bug?: \n aren't converted to <br>
            text = h.process(info.changelog.toUtf8());

            // Don't convert if there's an HTML tag right after (Qt rendering weirdness)
            text.remove(QRegularExpression("(\n+)(?=<)"));
            text.replace('\n', "<br>");

            break;
        }
        default:
            break;
    }

    changelog_area->setHtml(text);
    changelog_area->setOpenExternalLinks(true);
    changelog_area->setLineWrapMode(QTextBrowser::LineWrapMode::WidgetWidth);
    changelog_area->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);

    ui->modTreeWidget->setItemWidget(changelog, 0, changelog_area);

    ui->modTreeWidget->addTopLevelItem(itehello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_top);
}

auto ModUpdateDialog::getTasks() -> const QList<ModDownloadTask*>
{
    QList<ModDownloadTask*> list;

    auto* item = ui->modTreeWidget->topLevelItem(0);

    for (int i = 1; item != nullptr; ++i) {
        if (item->checkState(0) == Qt::CheckState::Checked) {
            list.push_back(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.find(item->text(0)).value());
        }

        item = ui->modTreeWidget->topLevelItem(i);
    }

    return list;
}
