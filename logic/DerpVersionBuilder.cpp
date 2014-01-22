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

#include "DerpVersionBuilder.h"

#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QDir>
#include <QDebug>

#include "DerpVersion.h"
#include "DerpInstance.h"
#include "DerpRule.h"

DerpVersionBuilder::DerpVersionBuilder()
{

}

bool DerpVersionBuilder::build(DerpVersion *version, DerpInstance *instance, QWidget *widgetParent)
{
	DerpVersionBuilder builder;
	builder.m_version = version;
	builder.m_instance = instance;
	builder.m_widgetParent = widgetParent;
	return builder.build();
}

bool DerpVersionBuilder::build()
{
	m_version->clear();

	QDir root(m_instance->instanceRoot());
	QDir patches(root.absoluteFilePath("patches/"));

	// version.json -> patches/*.json -> custom.json

	// version.json
	{
		QJsonObject obj;
		if (!read(QFileInfo(root.absoluteFilePath("version.json")), &obj))
		{
			return false;
		}
		if (!apply(obj))
		{
			return false;
		}
	}

	// patches/
	{
		// load all, put into map for ordering, apply in the right order

		QMap<int, QJsonObject> objects;
		for (auto info : patches.entryInfoList(QStringList() << "*.json", QDir::Files))
		{
			QJsonObject obj;
			if (!read(info, &obj))
			{
				return false;
			}
			if (!obj.contains("order") || !obj.value("order").isDouble())
			{
				QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("Missing or invalid 'order' in %1").arg(info.absoluteFilePath()));
				return false;
			}
			objects.insert(obj.value("order").toDouble(), obj);
		}
		for (auto object : objects.values())
		{
			if (!apply(object))
			{
				return false;
			}
		}
	}

	// custom.json
	{
		if (QFile::exists(root.absoluteFilePath("custom.json")))
		{
			QJsonObject obj;
			if (!read(QFileInfo(root.absoluteFilePath("custom.json")), &obj))
			{
				return false;
			}
			if (!apply(obj))
			{
				return false;
			}
		}
	}

	return true;
}

void applyString(const QJsonObject &obj, const QString &key, QString &out, const bool onlyOverride = true)
{
	if (obj.contains(key) && obj.value(key).isString())
	{
		out = obj.value(key).toString();
	}
	else if (!onlyOverride)
	{
		if (obj.contains("+" + key) && obj.value("+" + key).isString())
		{
			out += obj.value("+" + key).toString();
		}
		else if (obj.contains("-" + key) && obj.value("-" + key).isString())
		{
			out.remove(obj.value("-" + key).toString());
		}
	}
}
void applyString(const QJsonObject &obj, const QString &key, std::shared_ptr<DerpLibrary> lib, void(DerpLibrary::*func)(const QString &val))
{
	if (obj.contains(key) && obj.value(key).isString())
	{
		(lib.get()->*func)(obj.value(key).toString());
	}
}
bool DerpVersionBuilder::apply(const QJsonObject &object)
{
	applyString(object, "id", m_version->id);
	applyString(object, "mainClass", m_version->mainClass);
	applyString(object, "processArguments", m_version->processArguments, false);
	{
		const QString toCompare = m_version->processArguments.toLower();
		if (toCompare == "legacy")
		{
			m_version->minecraftArguments = " ${auth_player_name} ${auth_session}";
		}
		else if (toCompare == "username_session")
		{
			m_version->minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
		}
		else if (toCompare == "username_session_version")
		{
			m_version->minecraftArguments = "--username ${auth_player_name} --session ${auth_session} --version ${profile_name}";
		}
	}
	applyString(object, "minecraftArguments", m_version->minecraftArguments, false);
	applyString(object, "type", m_version->type);
	applyString(object, "releaseTime", m_version->releaseTime);
	applyString(object, "time", m_version->time);
	applyString(object, "assets", m_version->assets);
	{
		if (m_version->assets.isEmpty())
		{
			m_version->assets = "legacy";
		}
	}
	if (object.contains("minimumLauncherVersion"))
	{
		auto minLauncherVersionVal = object.value("minimumLauncherVersion");
		if (minLauncherVersionVal.isDouble())
		{
			m_version->minimumLauncherVersion = minLauncherVersionVal.toDouble();
		}
	}

	// libraries
	if (object.contains("libraries"))
	{
		m_version->libraries.clear();
		auto librariesValue = object.value("libraries");
		if (!librariesValue.isArray())
		{
			QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("One json files contains a libraries field, but it's not an array"));
			return false;
		}
		auto array = librariesValue.toArray();
		for (auto libVal : array)
		{
			if (libVal.isObject())
			{
				if (!applyLibrary(libVal.toObject(), Override))
				{
					return false;
				}
			}

		}
	}

	// +libraries
	if (object.contains("+libraries"))
	{
		auto librariesValue = object.value("+libraries");
		if (!librariesValue.isArray())
		{
			QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("One json files contains a libraries field, but it's not an array"));
			return false;
		}
		for (auto libVal : librariesValue.toArray())
		{
			if (libVal.isObject())
			{
				applyLibrary(libVal.toObject(), Add);
			}

		}
	}

	// -libraries
	if (object.contains("-libraries"))
	{
		auto librariesValue = object.value("-libraries");
		if (!librariesValue.isArray())
		{
			QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("One json files contains a libraries field, but it's not an array"));
			return false;
		}
		for (auto libVal : librariesValue.toArray())
		{
			if (libVal.isObject())
			{
				applyLibrary(libVal.toObject(), Remove);
			}

		}
	}

	return true;
}

int findLibrary(QList<std::shared_ptr<DerpLibrary> > haystack, const QString &needle)
{
	for (int i = 0; i < haystack.size(); ++i)
	{
		if (QRegExp(needle, Qt::CaseSensitive, QRegExp::WildcardUnix).indexIn(haystack.at(i)->rawName()) != -1)
		{
			return i;
		}
	}
	return -1;
}

bool DerpVersionBuilder::applyLibrary(const QJsonObject &lib, const DerpVersionBuilder::Type type)
{
	// Library name
	auto nameVal = lib.value("name");
	if (!nameVal.isString())
	{
		return false;
	}
	auto name = nameVal.toString();

	if (type == Remove)
	{
		int index = findLibrary(m_version->libraries, name);
		if (index >= 0)
		{
			m_version->libraries.removeAt(index);
		}
		return true;
	}

	if (type == Add && !lib.contains("insert"))
	{
		return false;
	}

	std::shared_ptr<DerpLibrary> library;

	if (lib.value("insert").toString() == "apply" && type == Add)
	{
		library = m_version->libraries[findLibrary(m_version->libraries, name)];
	}
	else
	{
		library.reset(new DerpLibrary(nameVal.toString()));
	}

	applyString(lib, "url", library, &DerpLibrary::setBaseUrl);
	applyString(lib, "MMC-hint", library, &DerpLibrary::setHint);
	applyString(lib, "MMC-absulute_url", library, &DerpLibrary::setAbsoluteUrl);
	applyString(lib, "MMC-absoluteUrl", library, &DerpLibrary::setAbsoluteUrl);

	auto extractVal = lib.value("extract");
	if (extractVal.isObject())
	{
		QStringList excludes;
		auto extractObj = extractVal.toObject();
		auto excludesVal = extractObj.value("exclude");
		if (excludesVal.isArray())
		{
			auto excludesList = excludesVal.toArray();
			for (auto excludeVal : excludesList)
			{
				if (excludeVal.isString())
				{
					excludes.append(excludeVal.toString());
				}
			}
			library->extract_excludes = excludes;
		}
	}

	auto nativesVal = lib.value("natives");
	if (nativesVal.isObject())
	{
		library->setIsNative();
		auto nativesObj = nativesVal.toObject();
		for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
		{
			auto osType = OpSys_fromString(it.key());
			if (osType == Os_Other)
			{
				continue;
			}
			if (!it.value().isString())
			{
				continue;
			}
			library->addNative(osType, it.value().toString());
		}
	}

	if (lib.contains("rules"))
	{
		library->setRules(rulesFromJsonV4(lib));
	}
	library->finalize();
	if (type == Override)
	{
		qDebug() << "appending" << library->rawName();
		m_version->libraries.append(library);
	}
	else if (lib.value("insert").toString() != "apply")
	{
		if (lib.value("insert").toString() == "append")
		{
			m_version->libraries.append(library);
		}
		else if (lib.value("insert").toString() == "prepend")
		{
			m_version->libraries.prepend(library);
		}
		else if (lib.value("insert").isObject())
		{
			QJsonObject insertObj = lib.value("insert").toObject();
			if (insertObj.isEmpty())
			{
				QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("'insert' object empty"));
				return false;
			}
			const QString key = insertObj.keys().first();
			const QString value = insertObj.value(key).toString();
			const int index = findLibrary(m_version->libraries, value);
			if (index >= 0)
			{
				if (key == "before")
				{
					m_version->libraries.insert(index, library);
				}
				else if (key == "after")
				{
					m_version->libraries.insert(index + 1, library);
				}
				else
				{
					QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("Invalid value for 'insert': %1").arg(lib.value("insert").toString()));
					return false;
				}
			}
		}
		else
		{
			QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("Invalid value for 'insert': %1").arg(lib.value("insert").toString()));
			return false;
		}
	}
	return true;
}

bool DerpVersionBuilder::read(const QFileInfo &fileInfo, QJsonObject *out)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("Unable to open %1: %2").arg(file.fileName(), file.errorString()));
		return false;
	}
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		QMessageBox::critical(m_widgetParent, QObject::tr("Error"), QObject::tr("Unable to parse %1: %2 at %3").arg(file.fileName(), error.errorString()).arg(error.offset));
		return false;
	}
	*out = doc.object();
	return true;
}
