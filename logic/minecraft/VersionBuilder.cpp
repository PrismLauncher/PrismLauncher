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
#include <qresource.h>
#include <modutils.h>

#include "MultiMC.h"
#include "logic/minecraft/VersionBuilder.h"
#include "logic/minecraft/VersionFinal.h"
#include "logic/minecraft/OneSixRule.h"
#include "logic/minecraft/VersionPatch.h"
#include "logic/minecraft/VersionFile.h"
#include "VersionBuildError.h"
#include "MinecraftVersionList.h"

#include "logic/OneSixInstance.h"
#include "logic/MMCJson.h"

#include "logger/QsLog.h"

VersionBuilder::VersionBuilder()
{
}

void VersionBuilder::build(VersionFinal *version, OneSixInstance *instance,
						   const QStringList &external)
{
	VersionBuilder builder;
	builder.m_version = version;
	builder.m_instance = instance;
	builder.external_patches = external;
	builder.buildInternal();
}

void VersionBuilder::readJsonAndApplyToVersion(VersionFinal *version, const QJsonObject &obj)
{
	VersionBuilder builder;
	builder.m_version = version;
	builder.m_instance = 0;
	builder.readJsonAndApply(obj);
}

void VersionBuilder::buildFromCustomJson()
{
	QLOG_INFO() << "Building version from custom.json within the instance.";
	QLOG_INFO() << "Reading custom.json";
	auto file = parseJsonFile(QFileInfo(instance_root.absoluteFilePath("custom.json")), false);
	file->name = "custom.json";
	file->filename = "custom.json";
	file->fileId = "org.multimc.custom.json";
	file->order = -1;
	file->version = QString();
	m_version->VersionPatches.append(file);
	// QObject::tr("The version descriptors of this instance are not compatible with the
	// current version of MultiMC"));
	// QObject::tr("Error while applying %1. Please check MultiMC-0.log for more info.")
	// some final touches
	m_version->finalize();
	return;
}

void VersionBuilder::buildFromVersionJson()
{
	QLOG_INFO() << "Building version from version.json and patches within the instance.";
	QLOG_INFO() << "Reading version.json";
	auto file = parseJsonFile(QFileInfo(instance_root.absoluteFilePath("version.json")), false);
	file->name = "Minecraft";
	file->fileId = "org.multimc.version.json";
	file->order = -1;
	file->version = m_instance->intendedVersionId();
	file->mcVersion = m_instance->intendedVersionId();
	m_version->VersionPatches.append(file);

	// load all patches, put into map for ordering, apply in the right order
	readInstancePatches();

	// some final touches
	m_version->finalize();
}

void VersionBuilder::readInstancePatches()
{
	QDir patches(instance_root.absoluteFilePath("patches/"));
	QMap<int, QPair<QString, VersionFilePtr>> files;
	for (auto info : patches.entryInfoList(QStringList() << "*.json", QDir::Files))
	{
		QLOG_INFO() << "Reading" << info.fileName();
		auto file = parseJsonFile(info, true);
		if(file->fileId == "net.minecraft")
			continue;
		if(file->fileId == "org.lwjgl")
			continue;
		if (files.contains(file->order))
		{
			throw VersionBuildError(QObject::tr("%1 has the same order as %2")
										.arg(file->fileId, files[file->order].second->fileId));
		}
		files.insert(file->order, qMakePair(info.fileName(), file));
	}
	for (auto order : files.keys())
	{
		auto &filePair = files[order];
		m_version->VersionPatches.append(filePair.second);
	}
}

void VersionBuilder::buildFromExternalPatches()
{
	QLOG_INFO() << "Building version from external files.";
	int externalOrder = -1;
	for (auto fileName : external_patches)
	{
		QLOG_INFO() << "Reading" << fileName;
		auto file = parseJsonFile(QFileInfo(fileName), false, fileName.endsWith("pack.json"));
		file->name = QFileInfo(fileName).fileName();
		file->fileId = "org.multimc.external." + file->name;
		file->order = (externalOrder += 1);
		file->version = QString();
		file->mcVersion = QString();
		m_version->VersionPatches.append(file);
	}
	// some final touches
	m_version->finalize();
}

void VersionBuilder::buildFromMultilayer()
{
	QLOG_INFO() << "Building version from multilayered sources.";
	// just the builtin stuff for now
	auto minecraftList = MMC->minecraftlist();
	auto mcversion = minecraftList->findVersion(m_instance->intendedVersionId());
	auto minecraftPatch = std::dynamic_pointer_cast<VersionPatch>(mcversion);
	if(!minecraftPatch)
	{
		throw VersionIncomplete("net.minecraft");
	}
	minecraftPatch->setOrder(-2);
	m_version->VersionPatches.append(minecraftPatch);

	QResource LWJGL(":/versions/LWJGL/2.9.1.json");
	auto lwjgl = parseJsonFile(LWJGL.absoluteFilePath(), false, false);
	auto lwjglPatch = std::dynamic_pointer_cast<VersionPatch>(lwjgl);
	if(!lwjglPatch)
	{
		throw VersionIncomplete("org.lwjgl");
	}
	lwjglPatch->setOrder(-1);
	m_version->VersionPatches.append(lwjglPatch);

	// load all patches, put into map for ordering, apply in the right order
	readInstancePatches();

	m_version->finalize();
}

void VersionBuilder::buildInternal()
{
	m_version->VersionPatches.clear();
	instance_root = QDir(m_instance->instanceRoot());
	// if we do external files, do just those.
	if (!external_patches.isEmpty())
	{
		buildFromExternalPatches();
	}
	// else, if there's custom json, we just do that.
	else if (QFile::exists(instance_root.absoluteFilePath("custom.json")))
	{
		buildFromCustomJson();
	}
	// version.json -> patches/*.json
	else if (QFile::exists(instance_root.absoluteFilePath("version.json")))
	{
		buildFromVersionJson();
	}
	else
	{
		buildFromMultilayer();
	}
}

void VersionBuilder::readJsonAndApply(const QJsonObject &obj)
{
	m_version->clear();

	auto file = VersionFile::fromJson(QJsonDocument(obj), QString(), false);
	// QObject::tr("Error while reading. Please check MultiMC-0.log for more info."));

	file->applyTo(m_version);
	m_version->VersionPatches.append(file);
	// QObject::tr("Error while applying. Please check MultiMC-0.log for more info."));
	// QObject::tr("The version descriptors of this instance are not compatible with the current
	// version of MultiMC"));
}

VersionFilePtr VersionBuilder::parseJsonFile(const QFileInfo &fileInfo, const bool requireOrder,
											 bool isFTB)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		throw JSONValidationError(QObject::tr("Unable to open the version file %1: %2.")
									  .arg(fileInfo.fileName(), file.errorString()));
	}
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		throw JSONValidationError(
			QObject::tr("Unable to process the version file %1: %2 at %3.")
				.arg(fileInfo.fileName(), error.errorString())
				.arg(error.offset));
	}
	return VersionFile::fromJson(doc, file.fileName(), requireOrder, isFTB);
	// QObject::tr("Error while reading %1. Please check MultiMC-0.log for more
	// info.").arg(file.fileName());
}

VersionFilePtr VersionBuilder::parseBinaryJsonFile(const QFileInfo &fileInfo)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		throw JSONValidationError(QObject::tr("Unable to open the version file %1: %2.")
									  .arg(fileInfo.fileName(), file.errorString()));
	}
	QJsonDocument doc = QJsonDocument::fromBinaryData(file.readAll());
	file.close();
	if (doc.isNull())
	{
		file.remove();
		throw JSONValidationError(
			QObject::tr("Unable to process the version file %1.").arg(fileInfo.fileName()));
	}
	return VersionFile::fromJson(doc, file.fileName(), false, false);
}

QMap<QString, int> VersionBuilder::readOverrideOrders(OneSixInstance *instance)
{
	QMap<QString, int> out;

	// make sure the order file exists
	if (!QDir(instance->instanceRoot()).exists("order.json"))
		return out;

	// and it can be opened
	QFile orderFile(instance->instanceRoot() + "/order.json");
	if (!orderFile.open(QFile::ReadOnly))
	{
		QLOG_ERROR() << "Couldn't open" << orderFile.fileName()
					 << " for reading:" << orderFile.errorString();
		QLOG_WARN() << "Ignoring overriden order";
		return out;
	}

	// and it's valid JSON
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(orderFile.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		QLOG_ERROR() << "Couldn't parse" << orderFile.fileName() << ":" << error.errorString();
		QLOG_WARN() << "Ignoring overriden order";
		return out;
	}

	// and then read it and process it if all above is true.
	try
	{
		auto obj = MMCJson::ensureObject(doc);
		for (auto it = obj.begin(); it != obj.end(); ++it)
		{
			if (it.key().startsWith("org.multimc."))
			{
				continue;
			}
			out.insert(it.key(), MMCJson::ensureInteger(it.value()));
		}
	}
	catch (JSONValidationError &err)
	{
		QLOG_ERROR() << "Couldn't parse" << orderFile.fileName() << ": bad file format";
		QLOG_WARN() << "Ignoring overriden order";
		return out;
	}
	return out;
}

bool VersionBuilder::writeOverrideOrders(const QMap<QString, int> &order,
										 OneSixInstance *instance)
{
	QJsonObject obj;
	for (auto it = order.cbegin(); it != order.cend(); ++it)
	{
		if (it.key().startsWith("org.multimc."))
		{
			continue;
		}
		obj.insert(it.key(), it.value());
	}
	QFile orderFile(instance->instanceRoot() + "/order.json");
	if (!orderFile.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Couldn't open" << orderFile.fileName()
					 << "for writing:" << orderFile.errorString();
		return false;
	}
	orderFile.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
	return true;
}
