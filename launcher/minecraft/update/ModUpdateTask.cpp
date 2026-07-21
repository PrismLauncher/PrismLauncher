#include "ModUpdateTask.h"

#include <ui/dialogs/ProgressDialog.h>
#include <ui/dialogs/ResourceUpdateDialog.h>
#include "Application.h"
#include "BuildConfig.h"
#include "launch/LaunchStep.h"
#include "minecraft/AssetsUtils.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModDetails.h"
#include "minecraft/mod/ModFolderModel.h"
#include "net/ApiDownload.h"
#include "net/ChecksumValidator.h"
#include "ui/dialogs/CustomMessageBox.h"

ModUpdateTask::ModUpdateTask(MinecraftInstance* inst, bool enabledModsOnly)
{
    m_instance = inst;
    m_enabledModsOnly = enabledModsOnly;
}

void ModUpdateTask::executeTask()
{
    setStatus(tr("Updating mods..."));
    qDebug() << "Updating mods...";

    if (m_instance->typeName() != "Minecraft") {
        return;  // this is a null instance or a legacy instance
    }

    auto* profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
    if (!profile->getModLoaders().has_value()) {
        emitFailed(tr("Mod updates are unavailable when mod loader is missing!"));
        return;
    }

    if (APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        emitFailed(tr("Mod updates are unavailable when metadata is disabled!"));
        return;
    }

    // ResourceUpdateDialog only accepts Resource,
    // so we convert the Mod* list to a Resource* list with a simple static_cast
    auto model = m_instance->loaderModList();
    auto _modsList = model->allMods();
    QList<Resource*> modsList;
    modsList.reserve(_modsList.size());
    for (auto mod : _modsList) {
        modsList.append(static_cast<Resource*>(mod));
    }

    // Filter out disabled mods if "AutomaticallyUpdateModsEnabled" is true
    if (m_enabledModsOnly) {
        modsList.erase(std::remove_if(modsList.begin(), modsList.end(), [](Resource* resource) { return !resource->enabled(); }),
                       modsList.end());
    }

    // Spawn ResourceUpdateDialog to handle mod updates
    // 99% copied from ModFolderPage.cpp
    auto parent = QApplication::activeWindow();
    ResourceUpdateDialog updateDialog =
        ResourceUpdateDialog(parent, m_instance, model, modsList, m_includeDeps, profile->getModLoadersList());
    updateDialog.checkCandidates();

    if (updateDialog.aborted()) {
        CustomMessageBox::selectable(parent, tr("Aborted"), tr("The mod updater was aborted!"), QMessageBox::Warning)->show();
        emitAborted();
        return;
    }
    if (updateDialog.noUpdates()) {
        QString message{ tr("'%1' is up-to-date! :)").arg(modsList.front()->name()) };
        if (modsList.size() > 1) {
            if (!m_enabledModsOnly) {
                message = tr("All mods are up-to-date! :)");
            } else {
                message = tr("All enabled mods are up-to-date! :)");
            }
        }
        CustomMessageBox::selectable(parent, tr("Update checker"), message)->exec();
        emitSucceeded();
        return;
    }

    if (updateDialog.exec() != 0) {
        auto* tasks = new ConcurrentTask("Download Mods", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        connect(tasks, &Task::failed, [this, parent, tasks](const QString& reason) {
            CustomMessageBox::selectable(parent, tr("Error"), reason, QMessageBox::Critical)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::aborted, [this, parent, tasks]() {
            CustomMessageBox::selectable(parent, tr("Aborted"), tr("Download stopped by user."), QMessageBox::Information)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::succeeded, [this, parent, tasks]() {
            QStringList warnings = tasks->warnings();
            if (warnings.count()) {
                CustomMessageBox::selectable(parent, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }
            tasks->deleteLater();
        });

        for (const auto& task : updateDialog.getTasks()) {
            tasks->addTask(task);
        }

        ProgressDialog loadDialog(parent);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(tasks);
    }
    model->update();
    qDebug() << "Finished updating mods...";
    emitSucceeded();
}

bool ModUpdateTask::canAbort() const
{
    return true;
}

bool ModUpdateTask::abort()
{
    return true;
}
