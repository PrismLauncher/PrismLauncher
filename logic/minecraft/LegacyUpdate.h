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

#pragma once

#include <QObject>
#include <QList>
#include <QUrl>

#include "logic/net/NetJob.h"
#include "logic/tasks/Task.h"
#include "logic/minecraft/VersionFilterData.h"

class MinecraftVersion;
class BaseInstance;
class QuaZip;
class Mod;

class LegacyUpdate : public Task
{
	Q_OBJECT
public:
	explicit LegacyUpdate(BaseInstance *inst, QObject *parent = 0);
	virtual void executeTask();

private
slots:
	void lwjglStart();
	void lwjglFinished(QNetworkReply *);
	void lwjglFailed();

	void jarStart();
	void jarFinished();
	void jarFailed();

	void fmllibsStart();
	void fmllibsFinished();
	void fmllibsFailed();

	void extractLwjgl();

	void ModTheJar();

private:

	std::shared_ptr<QNetworkReply> m_reply;

	// target version, determined during this task
	// MinecraftVersion *targetVersion;
	QString lwjglURL;
	QString lwjglVersion;

	QString lwjglTargetPath;
	QString lwjglNativesPath;

private:
	NetJobPtr legacyDownloadJob;
	BaseInstance *m_inst = nullptr;
	QList<FMLlib> fmlLibsToProcess;
};
