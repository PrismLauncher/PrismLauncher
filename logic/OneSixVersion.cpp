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

#include "logic/OneSixVersion.h"
#include "logic/OneSixLibrary.h"
#include "logic/OneSixRule.h"

#include "logger/QsLog.h"

std::shared_ptr<OneSixVersion> fromJsonV4(QJsonObject root,
										  std::shared_ptr<OneSixVersion> fullVersion)
{
	fullVersion->id = root.value("id").toString();

	fullVersion->mainClass = root.value("mainClass").toString();
	auto procArgsValue = root.value("processArguments");
	if (procArgsValue.isString())
	{
		fullVersion->processArguments = procArgsValue.toString();
		QString toCompare = fullVersion->processArguments.toLower();
		if (toCompare == "legacy")
		{
			fullVersion->minecraftArguments = " ${auth_player_name} ${auth_session}";
		}
		else if (toCompare == "username_session")
		{
			fullVersion->minecraftArguments =
				"--username ${auth_player_name} --session ${auth_session}";
		}
		else if (toCompare == "username_session_version")
		{
			fullVersion->minecraftArguments = "--username ${auth_player_name} "
											  "--session ${auth_session} "
											  "--version ${profile_name}";
		}
	}

	auto minecraftArgsValue = root.value("minecraftArguments");
	if (minecraftArgsValue.isString())
	{
		fullVersion->minecraftArguments = minecraftArgsValue.toString();
	}

	auto minecraftTypeValue = root.value("type");
	if (minecraftTypeValue.isString())
	{
		fullVersion->type = minecraftTypeValue.toString();
	}

	fullVersion->releaseTime = root.value("releaseTime").toString();
	fullVersion->time = root.value("time").toString();

	auto assetsID = root.value("assets");
	if (assetsID.isString())
	{
		fullVersion->assets = assetsID.toString();
	}
	else
	{
		fullVersion->assets = "legacy";
	}

	QLOG_DEBUG() << "Assets version:" << fullVersion->assets;

	// Iterate through the list, if it's a list.
	auto librariesValue = root.value("libraries");
	if (!librariesValue.isArray())
		return fullVersion;

	QJsonArray libList = root.value("libraries").toArray();
	for (auto libVal : libList)
	{
		if (!libVal.isObject())
		{
			continue;
		}

		QJsonObject libObj = libVal.toObject();

		// Library name
		auto nameVal = libObj.value("name");
		if (!nameVal.isString())
			continue;
		std::shared_ptr<OneSixLibrary> library(new OneSixLibrary(nameVal.toString()));

		auto urlVal = libObj.value("url");
		if (urlVal.isString())
		{
			library->setBaseUrl(urlVal.toString());
		}
		auto hintVal = libObj.value("MMC-hint");
		if (hintVal.isString())
		{
			library->setHint(hintVal.toString());
		}
		auto urlAbsVal = libObj.value("MMC-absoluteUrl");
		auto urlAbsuVal = libObj.value("MMC-absulute_url"); // compatibility
		if (urlAbsVal.isString())
		{
			library->setAbsoluteUrl(urlAbsVal.toString());
		}
		else if (urlAbsuVal.isString())
		{
			library->setAbsoluteUrl(urlAbsuVal.toString());
		}
		// Extract excludes (if any)
		auto extractVal = libObj.value("extract");
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
						excludes.append(excludeVal.toString());
				}
				library->extract_excludes = excludes;
			}
		}

		auto nativesVal = libObj.value("natives");
		if (nativesVal.isObject())
		{
			library->setIsNative();
			auto nativesObj = nativesVal.toObject();
			auto iter = nativesObj.begin();
			while (iter != nativesObj.end())
			{
				auto osType = OpSys_fromString(iter.key());
				if (osType == Os_Other)
					continue;
				if (!iter.value().isString())
					continue;
				library->addNative(osType, iter.value().toString());
				iter++;
			}
		}
		library->setRules(rulesFromJsonV4(libObj));
		library->finalize();
		fullVersion->libraries.append(library);
	}
	return fullVersion;
}

std::shared_ptr<OneSixVersion> OneSixVersion::fromJson(QJsonObject root)
{
	std::shared_ptr<OneSixVersion> readVersion(new OneSixVersion());
	int launcher_ver = readVersion->minimumLauncherVersion =
		root.value("minimumLauncherVersion").toDouble();

	// ADD MORE HERE :D
	if (launcher_ver > 0 && launcher_ver <= 13)
		return fromJsonV4(root, readVersion);
	else
	{
		return std::shared_ptr<OneSixVersion>();
	}
}

std::shared_ptr<OneSixVersion> OneSixVersion::fromFile(QString filepath)
{
	QFile file(filepath);
	if (!file.open(QIODevice::ReadOnly))
	{
		return std::shared_ptr<OneSixVersion>();
	}

	auto data = file.readAll();
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		return std::shared_ptr<OneSixVersion>();
	}

	if (!jsonDoc.isObject())
	{
		return std::shared_ptr<OneSixVersion>();
	}
	QJsonObject root = jsonDoc.object();
	auto version = fromJson(root);
	if (version)
		version->original_file = filepath;
	return version;
}

bool OneSixVersion::toOriginalFile()
{
	if (original_file.isEmpty())
		return false;
	QSaveFile file(original_file);
	if (!file.open(QIODevice::WriteOnly))
	{
		return false;
	}
	// serialize base attributes (those we care about anyway)
	QJsonObject root;
	root.insert("minecraftArguments", minecraftArguments);
	root.insert("mainClass", mainClass);
	root.insert("minimumLauncherVersion", minimumLauncherVersion);
	root.insert("time", time);
	root.insert("id", id);
	root.insert("type", type);
	// screw processArguments
	root.insert("releaseTime", releaseTime);
	QJsonArray libarray;
	for (const auto &lib : libraries)
	{
		libarray.append(lib->toJson());
	}
	if (libarray.count())
		root.insert("libraries", libarray);
	QJsonDocument doc(root);
	file.write(doc.toJson());
	return file.commit();
}

QList<std::shared_ptr<OneSixLibrary>> OneSixVersion::getActiveNormalLibs()
{
	QList<std::shared_ptr<OneSixLibrary>> output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && !lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

QList<std::shared_ptr<OneSixLibrary>> OneSixVersion::getActiveNativeLibs()
{
	QList<std::shared_ptr<OneSixLibrary>> output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

void OneSixVersion::externalUpdateStart()
{
	beginResetModel();
}

void OneSixVersion::externalUpdateFinish()
{
	endResetModel();
}

QVariant OneSixVersion::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= libraries.size())
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return libraries[row]->name();
		case 1:
			return libraries[row]->type();
		case 2:
			return libraries[row]->version();
		default:
			return QVariant();
		}
	}
	return QVariant();
}

Qt::ItemFlags OneSixVersion::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	int row = index.row();
	if (libraries[row]->isActive())
	{
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
	}
	else
	{
		return Qt::ItemNeverHasChildren;
	}
	// return QAbstractListModel::flags(index);
}

QVariant OneSixVersion::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();
	switch (section)
	{
	case 0:
		return QString("Name");
	case 1:
		return QString("Type");
	case 2:
		return QString("Version");
	default:
		return QString();
	}
}

int OneSixVersion::rowCount(const QModelIndex &parent) const
{
	return libraries.size();
}

int OneSixVersion::columnCount(const QModelIndex &parent) const
{
	return 3;
}
