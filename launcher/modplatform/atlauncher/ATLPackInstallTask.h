/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 * Copyright 2021 Petr Mrazek <peterix@gmail.com>
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

#include <meta/VersionList.h>
#include "ATLPackManifest.h"

#include "InstanceTask.h"
#include "net/NetJob.h"
#include "settings/INISettingsObject.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "meta/Version.h"

#include <nonstd/optional>

namespace ATLauncher {

class UserInteractionSupport {

public:
    /**
     * Requests a user interaction to select which optional mods should be installed.
     */
    virtual QVector<QString> chooseOptionalMods(PackVersion version, QVector<ATLauncher::VersionMod> mods) = 0;

    /**
     * Requests a user interaction to select a component version from a given version list
     * and constrained to a given Minecraft version.
     */
    virtual QString chooseVersion(Meta::VersionListPtr vlist, QString minecraftVersion) = 0;

};

class PackInstallTask : public InstanceTask
{
Q_OBJECT

public:
    explicit PackInstallTask(UserInteractionSupport *support, QString pack, QString version);
    virtual ~PackInstallTask(){}

    bool canAbort() const override { return true; }
    bool abort() override;

protected:
    virtual void executeTask() override;

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

    void onModsDownloaded();
    void onModsExtracted();

private:
    QString getDirForModType(ModType type, QString raw);
    QString getVersionForLoader(QString uid);
    QString detectLibrary(VersionLibrary library);

    bool createLibrariesComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile);
    bool createPackComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile);

    void installConfigs();
    void extractConfigs();
    void downloadMods();
    bool extractMods(
        const QMap<QString, VersionMod> &toExtract,
        const QMap<QString, VersionMod> &toDecomp,
        const QMap<QString, QString> &toCopy
    );
    void install();

private:
    UserInteractionSupport *m_support;

    bool abortable = false;

    NetJob::Ptr jobPtr;
    QByteArray response;

    QString m_pack;
    QString m_version_name;
    PackVersion m_version;

    QMap<QString, VersionMod> modsToExtract;
    QMap<QString, VersionMod> modsToDecomp;
    QMap<QString, QString> modsToCopy;

    QString archivePath;
    QStringList jarmods;
    Meta::VersionPtr minecraftVersion;
    QMap<QString, Meta::VersionPtr> componentsToInstall;

    QFuture<nonstd::optional<QStringList>> m_extractFuture;
    QFutureWatcher<nonstd::optional<QStringList>> m_extractFutureWatcher;

    QFuture<bool> m_modExtractFuture;
    QFutureWatcher<bool> m_modExtractFutureWatcher;

};

}
