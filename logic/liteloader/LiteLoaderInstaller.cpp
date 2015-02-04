/* Copyright 2013-2015 MultiMC Contributors
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

#include <QDebug>

#include "logic/minecraft/MinecraftProfile.h"
#include "logic/minecraft/OneSixLibrary.h"
#include "logic/minecraft/OneSixInstance.h"
#include "logic/liteloader/LiteLoaderVersionList.h"

LiteLoaderInstaller::LiteLoaderInstaller() : BaseInstaller()
{
}

void LiteLoaderInstaller::prepare(LiteLoaderVersionPtr version)
{
	m_version = version;
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

	for (auto rawLibrary : m_version->libraries)
	{
		rawLibrary->insertType = RawLibrary::Prepend;
		OneSixLibrary lib(rawLibrary);
		libraries.append(lib.toJson());
	}

	// liteloader
	{
		OneSixLibrary liteloaderLib("com.mumfrey:liteloader:" + m_version->version);
		liteloaderLib.setAbsoluteUrl(
			QString("http://dl.liteloader.com/versions/com/mumfrey/liteloader/%1/%2")
				.arg(m_version->mcVersion, m_version->file));
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
		qCritical() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	return true;
}

class LiteLoaderInstallTask : public Task
{
	Q_OBJECT
public:
	LiteLoaderInstallTask(LiteLoaderInstaller *installer, OneSixInstance *instance,
						  BaseVersionPtr version, QObject *parent)
		: Task(parent), m_installer(installer), m_instance(instance), m_version(version)
	{
	}

protected:
	void executeTask() override
	{
		LiteLoaderVersionPtr liteloaderVersion =
			std::dynamic_pointer_cast<LiteLoaderVersion>(m_version);
		if (!liteloaderVersion)
		{
			return;
		}
		m_installer->prepare(liteloaderVersion);
		if (!m_installer->add(m_instance))
		{
			emitFailed(tr("For reasons unknown, the LiteLoader installation failed. Check your "
						  "MultiMC log files for details."));
		}
		else
		{
			try
			{
				m_instance->reloadProfile();
				emitSucceeded();
			}
			catch (MMCError &e)
			{
				emitFailed(e.cause());
			}
			catch (...)
			{
				emitFailed(
					tr("Failed to load the version description file for reasons unknown."));
			}
		}
	}

private:
	LiteLoaderInstaller *m_installer;
	OneSixInstance *m_instance;
	BaseVersionPtr m_version;
};

ProgressProvider *LiteLoaderInstaller::createInstallTask(OneSixInstance *instance,
														 BaseVersionPtr version,
														 QObject *parent)
{
	return new LiteLoaderInstallTask(this, instance, version, parent);
}

#include "LiteLoaderInstaller.moc"
