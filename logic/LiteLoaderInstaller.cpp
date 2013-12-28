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

#include "OneSixVersion.h"
#include "OneSixLibrary.h"

LiteLoaderInstaller::LiteLoaderInstaller()
{

}

bool hasLibrary(const QString &rawName, const QList<std::shared_ptr<OneSixLibrary> > &libs)
{
	for (auto lib : libs)
	{
		if (lib->rawName() == rawName)
		{
			return true;
		}
	}
	return false;
}

bool LiteLoaderInstaller::apply(std::shared_ptr<OneSixVersion> to)
{
	to->externalUpdateStart();

	if (!hasLibrary("net.minecraft:launchwrapper:1.8", to->libraries))
	{
		std::shared_ptr<OneSixLibrary> lib(new OneSixLibrary("net.minecraft:launchwrapper:1.8"));
		lib->finalize();
		to->libraries.prepend(lib);
	}

	if (!hasLibrary("com.mumfrey:liteloader:1.6.4", to->libraries))
	{
		std::shared_ptr<OneSixLibrary> lib(new OneSixLibrary("com.mumfrey:liteloader:1.6.4"));
		lib->setBaseUrl("http://dl.liteloader.com/versions/");
		lib->finalize();
		to->libraries.prepend(lib);
	}

	to->mainClass = "net.minecraft.launchwrapper.Launch";
	to->minecraftArguments.append(" --tweakClass com.mumfrey.liteloader.launch.LiteLoaderTweaker");

	to->externalUpdateFinish();
	return to->toOriginalFile();
}
