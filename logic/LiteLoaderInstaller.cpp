/* Copyright 2013 MultiMC Contributors
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

#include "LiteLoaderInstaller.h"

#include "DerpVersion.h"
#include "DerpLibrary.h"

QMap<QString, QString> LiteLoaderInstaller::m_launcherWrapperVersionMapping;

LiteLoaderInstaller::LiteLoaderInstaller(const QString &mcVersion) : m_mcVersion(mcVersion)
{
	if (m_launcherWrapperVersionMapping.isEmpty())
	{
		m_launcherWrapperVersionMapping["1.6.2"] = "1.3";
		m_launcherWrapperVersionMapping["1.6.4"] = "1.8";
		//m_launcherWrapperVersionMapping["1.7.2"] = "1.8";
		//m_launcherWrapperVersionMapping["1.7.4"] = "1.8";
	}
}

bool LiteLoaderInstaller::canApply() const
{
	return m_launcherWrapperVersionMapping.contains(m_mcVersion);
}

bool LiteLoaderInstaller::apply(std::shared_ptr<DerpVersion> to)
{
	// DERPFIX

	applyLaunchwrapper(to);
	applyLiteLoader(to);

	to->mainClass = "net.minecraft.launchwrapper.Launch";
	if (!to->minecraftArguments.contains(
			 " --tweakClass com.mumfrey.liteloader.launch.LiteLoaderTweaker"))
	{
		to->minecraftArguments.append(
			" --tweakClass com.mumfrey.liteloader.launch.LiteLoaderTweaker");
	}

	return true;
}

void LiteLoaderInstaller::applyLaunchwrapper(std::shared_ptr<DerpVersion> to)
{
	const QString intendedVersion = m_launcherWrapperVersionMapping[m_mcVersion];

	QMutableListIterator<std::shared_ptr<DerpLibrary>> it(to->libraries);
	while (it.hasNext())
	{
		it.next();
		if (it.value()->rawName().startsWith("net.minecraft:launchwrapper:"))
		{
			if (it.value()->version() >= intendedVersion)
			{
				return;
			}
			else
			{
				it.remove();
			}
		}
	}

	std::shared_ptr<DerpLibrary> lib(new DerpLibrary(
		"net.minecraft:launchwrapper:" + m_launcherWrapperVersionMapping[m_mcVersion]));
	lib->finalize();
	to->libraries.prepend(lib);
}

void LiteLoaderInstaller::applyLiteLoader(std::shared_ptr<DerpVersion> to)
{
	QMutableListIterator<std::shared_ptr<DerpLibrary>> it(to->libraries);
	while (it.hasNext())
	{
		it.next();
		if (it.value()->rawName().startsWith("com.mumfrey:liteloader:"))
		{
			it.remove();
		}
	}

	std::shared_ptr<DerpLibrary> lib(
		new DerpLibrary("com.mumfrey:liteloader:" + m_mcVersion));
	lib->setBaseUrl("http://dl.liteloader.com/versions/");
	lib->finalize();
	to->libraries.prepend(lib);
}
