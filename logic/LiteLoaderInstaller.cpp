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

#include <QJsonArray>
#include <QJsonDocument>

#include "logger/QsLog.h"

#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include "OneSixInstance.h"

QMap<QString, QString> LiteLoaderInstaller::m_launcherWrapperVersionMapping;

LiteLoaderInstaller::LiteLoaderInstaller()
	: BaseInstaller()
{
	if (m_launcherWrapperVersionMapping.isEmpty())
	{
		m_launcherWrapperVersionMapping["1.6.2"] = "1.3";
		m_launcherWrapperVersionMapping["1.6.4"] = "1.8";
		//m_launcherWrapperVersionMapping["1.7.2"] = "1.8";
		//m_launcherWrapperVersionMapping["1.7.4"] = "1.8";
	}
}

bool LiteLoaderInstaller::canApply(OneSixInstance *instance) const
{
	return m_launcherWrapperVersionMapping.contains(instance->intendedVersionId());
}

bool LiteLoaderInstaller::add(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}

	QJsonObject obj;

	obj.insert("mainClass", QString("net.minecraft.launchwrapper.Launch"));
	obj.insert("+tweakers", QJsonArray::fromStringList(QStringList() << "com.mumfrey.liteloader.launch.LiteLoaderTweaker"));
	obj.insert("order", 10);

	QJsonArray libraries;

	// launchwrapper
	{
		OneSixLibrary launchwrapperLib("net.minecraft:launchwrapper:" + m_launcherWrapperVersionMapping[to->intendedVersionId()]);
		launchwrapperLib.finalize();
		QJsonObject lwLibObj = launchwrapperLib.toJson();
		lwLibObj.insert("insert", QString("prepend"));
		libraries.append(lwLibObj);
	}

	// liteloader
	{
		OneSixLibrary liteloaderLib("com.mumfrey:liteloader:" + to->intendedVersionId());
		liteloaderLib.setBaseUrl("http://dl.liteloader.com/versions/");
		liteloaderLib.finalize();
		QJsonObject llLibObj = liteloaderLib.toJson();
		llLibObj.insert("insert", QString("prepend"));
		llLibObj.insert("MMC-depend", QString("hard"));
		libraries.append(llLibObj);
	}

	obj.insert("+libraries", libraries);
	obj.insert("name", QString("LiteLoader"));
	obj.insert("fileId", id());
	obj.insert("version", to->intendedVersionId());
	obj.insert("mcVersion", to->intendedVersionId());

	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName() << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	return true;
}
