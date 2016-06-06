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

#include "minecraft/MinecraftProfile.h"
#include "minecraft/Library.h"
#include "minecraft/onesix/OneSixInstance.h"
#include <minecraft/onesix/OneSixVersionFormat.h>
#include "minecraft/liteloader/LiteLoaderVersionList.h"
#include "Exception.h"

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
	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		qCritical() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(OneSixVersionFormat::versionFileToJson(m_version->getVersionFile(), true).toJson());
	file.close();

	return true;
}

class LiteLoaderInstallTask : public Task
{
	Q_OBJECT
public:
	LiteLoaderInstallTask(LiteLoaderInstaller *installer, OneSixInstance *instance, BaseVersionPtr version, QObject *parent)
		: Task(parent), m_installer(installer), m_instance(instance), m_version(version)
	{
	}

protected:
	void executeTask() override
	{
		LiteLoaderVersionPtr liteloaderVersion = std::dynamic_pointer_cast<LiteLoaderVersion>(m_version);
		if (!liteloaderVersion)
		{
			return;
		}
		m_installer->prepare(liteloaderVersion);
		if (!m_installer->add(m_instance))
		{
			emitFailed(tr("For reasons unknown, the LiteLoader installation failed. Check your MultiMC log files for details."));
			return;
		}
		try
		{
			m_instance->reloadProfile();
			emitSucceeded();
		}
		catch (Exception &e)
		{
			emitFailed(e.cause());
		}
		catch (...)
		{
			emitFailed(tr("Failed to load the version description file for reasons unknown."));
		}
	}

private:
	LiteLoaderInstaller *m_installer;
	OneSixInstance *m_instance;
	BaseVersionPtr m_version;
};

Task *LiteLoaderInstaller::createInstallTask(OneSixInstance *instance, BaseVersionPtr version, QObject *parent)
{
	return new LiteLoaderInstallTask(this, instance, version, parent);
}

#include "LiteLoaderInstaller.moc"
