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

#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QDir>
#include <qresource.h>
#include <modutils.h>
#include <pathutils.h>

#include "MultiMC.h"
#include "logic/minecraft/VersionBuilder.h"
#include "logic/minecraft/MinecraftProfile.h"
#include "logic/minecraft/OneSixRule.h"
#include "logic/minecraft/ProfilePatch.h"
#include "logic/minecraft/VersionFile.h"
#include "VersionBuildError.h"
#include "MinecraftVersionList.h"
#include "ProfileUtils.h"

#include "logic/OneSixInstance.h"
#include "logic/MMCJson.h"

#include "logger/QsLog.h"

VersionBuilder::VersionBuilder()
{
}

void VersionBuilder::build(MinecraftProfile *version, OneSixInstance *instance)
{
	VersionBuilder builder;
	builder.m_version = version;
	builder.m_instance = instance;
	builder.buildInternal();
}

void VersionBuilder::readJsonAndApplyToVersion(MinecraftProfile *version, const QJsonObject &obj)
{
	VersionBuilder builder;
	builder.m_version = version;
	builder.m_instance = 0;
	builder.readJsonAndApply(obj);
}

void VersionBuilder::readJsonAndApply(const QJsonObject &obj)
{
	m_version->clear();

	auto file = VersionFile::fromJson(QJsonDocument(obj), QString(), false);

	file->applyTo(m_version);
	m_version->appendPatch(file);
}


void VersionBuilder::readInstancePatches()
{

}

void VersionBuilder::buildInternal()
{

}

