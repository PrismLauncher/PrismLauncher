/* Copyright 2013-2017 MultiMC Contributors
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

#include "ModMinecraftJar.h"
#include "launch/LaunchTask.h"
#include "MMCZip.h"
#include "minecraft/OpSys.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/ComponentList.h"

void ModMinecraftJar::executeTask()
{
	auto m_inst = std::dynamic_pointer_cast<MinecraftInstance>(m_parent->instance());

	// nuke obsolete stripped jar(s) if needed
	if(!FS::ensureFolderPathExists(m_inst->binRoot()))
	{
		emitFailed(tr("Couldn't create the bin folder for Minecraft.jar"));
	}
	auto finalJarPath = QDir(m_inst->binRoot()).absoluteFilePath("minecraft.jar");
	QFile finalJar(finalJarPath);
	if(finalJar.exists())
	{
		if(!finalJar.remove())
		{
			emitFailed(tr("Couldn't remove stale jar file: %1").arg(finalJarPath));
			return;
		}
	}

	// create temporary modded jar, if needed
	auto profile = m_inst->getComponentList();
	auto jarMods = m_inst->getJarMods();
	if(jarMods.size())
	{
		auto mainJar = profile->getMainJar();
		QStringList jars, temp1, temp2, temp3, temp4;
		mainJar->getApplicableFiles(currentSystem, jars, temp1, temp2, temp3, m_inst->getLocalLibraryPath());
		auto sourceJarPath = jars[0];
		if(!MMCZip::createModdedJar(sourceJarPath, finalJarPath, jarMods))
		{
			emitFailed(tr("Failed to create the custom Minecraft jar file."));
			return;
		}
	}
	emitSucceeded();
}
