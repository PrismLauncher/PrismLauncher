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

#include "VersionFinal.h"
#include "OneSixLibrary.h"
#include "OneSixInstance.h"

LiteLoaderInstaller::LiteLoaderInstaller(LiteLoaderVersionPtr version)
	: BaseInstaller(), m_version(version)
{
}

bool LiteLoaderInstaller::add(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}

	QJsonObject obj;

	obj.insert("mainClass", QString("net.minecraft.launchwrapper.Launch"));
	obj.insert("+tweakers", QJsonArray::fromStringList(QStringList() << m_version->tweakClass));
	obj.insert("order", 10);

	QJsonArray libraries;

	for (auto libStr : m_version->libraries)
	{
		OneSixLibrary lib(libStr);
		lib.finalize();
		QJsonObject libObj = lib.toJson();
		libObj.insert("insert", QString("prepend"));
		libraries.append(libObj);
	}

	// liteloader
	{
		OneSixLibrary liteloaderLib("com.mumfrey:liteloader:" + m_version->version);
		liteloaderLib.setAbsoluteUrl(
			QString("http://dl.liteloader.com/versions/com/mumfrey/liteloader/%1/%2")
				.arg(m_version->mcVersion, m_version->file));
		liteloaderLib.finalize();
		QJsonObject llLibObj = liteloaderLib.toJson();
		llLibObj.insert("insert", QString("prepend"));
		llLibObj.insert("MMC-depend", QString("hard"));
		libraries.append(llLibObj);
	}

	obj.insert("+libraries", libraries);
	obj.insert("name", QString("LiteLoader"));
	obj.insert("fileId", id());
	obj.insert("version", m_version->version);
	obj.insert("mcVersion", to->intendedVersionId());

	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	return true;
}
