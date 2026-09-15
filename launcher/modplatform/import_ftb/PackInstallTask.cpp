// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "PackInstallTask.h"

#include <QtConcurrent>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/import_ftb/PackHelpers.h"
#include "settings/INISettingsObject.h"

namespace FTBImportAPP {

void PackInstallTask::executeTask()
{
    setStatus(tr("Copying files..."));
    setAbortable(false);
    progress(1, 2);

    m_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), [this] {
        FS::copy folderCopy(m_pack.path, FS::PathCombine(m_stagingPath, "minecraft"));
        return folderCopy();
    });
    connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &PackInstallTask::copySettings);
    connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &PackInstallTask::emitAborted);
    m_copyFutureWatcher.setFuture(m_copyFuture);
}

void PackInstallTask::copySettings()
{
    setStatus(tr("Copying settings..."));
    progress(2, 2);

    QString instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    m_instance =
        std::make_unique<MinecraftInstance>(m_globalSettings, std::make_unique<INISettingsObject>(instanceConfigPath), m_stagingPath);

    {
        SettingsObject::Lock const lock(m_instance->settings());
        m_instance->settings()->set("InstanceType", "OneSix");
        m_instance->settings()->set("totalTimePlayed", m_pack.totalPlayTime / 1000);

        if (m_pack.jvmArgs.isValid() && !m_pack.jvmArgs.toString().isEmpty()) {
            m_instance->settings()->set("OverrideJavaArgs", true);
            m_instance->settings()->set("JvmArgs", m_pack.jvmArgs.toString());
        }

        auto* components = m_instance->getPackProfile();
        components->buildingFromScratch();
        components->setComponentVersion("net.minecraft", m_pack.mcVersion, true);

        auto modloader = m_pack.loaderType;
        if (modloader.has_value()) {
            switch (modloader.value()) {
                case ModPlatform::NeoForge: {
                    components->setComponentVersion("net.neoforged", m_pack.loaderVersion, true);
                    break;
                }
                case ModPlatform::Forge: {
                    components->setComponentVersion("net.minecraftforge", m_pack.loaderVersion, true);
                    break;
                }
                case ModPlatform::Fabric: {
                    components->setComponentVersion("net.fabricmc.fabric-loader", m_pack.loaderVersion, true);
                    break;
                }
                case ModPlatform::Quilt: {
                    components->setComponentVersion("org.quiltmc.quilt-loader", m_pack.loaderVersion, true);
                    break;
                }
                default:
                    break;
            }
        }
        components->saveNow();

        m_instance->setName(name());
        if (m_instIcon == "default") {
            m_instIcon = "ftb_logo";
        }
        m_instance->setIconKey(m_instIcon);
    }
    downloadFiles(m_instance.get());
}

}  // namespace FTBImportAPP
