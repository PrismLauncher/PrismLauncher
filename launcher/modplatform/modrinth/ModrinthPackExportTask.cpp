// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ModrinthPackExportTask.h"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QMessageBox>
#include <QtConcurrentRun>
#include "Json.h"
#include "MMCZip.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"

const QStringList ModrinthPackExportTask::PREFIXES({ "mods/", "coremods/", "resourcepacks/", "texturepacks/", "shaderpacks/" });
const QStringList ModrinthPackExportTask::FILE_EXTENSIONS({ "jar", "litemod", "zip" });

ModrinthPackExportTask::ModrinthPackExportTask(const QString& name,
                                               const QString& version,
                                               const QString& summary,
                                               bool optionalFiles,
                                               InstancePtr instance,
                                               const QString& output,
                                               MMCZip::FilterFunction filter)
    : name(name)
    , version(version)
    , summary(summary)
    , optionalFiles(optionalFiles)
    , instance(instance)
    , mcInstance(dynamic_cast<MinecraftInstance*>(instance.get()))
    , gameRoot(instance->gameRoot())
    , output(output)
    , filter(filter)
{}

void ModrinthPackExportTask::executeTask()
{
    setStatus(tr("Searching for files..."));
    setProgress(0, 0);
    collectFiles();
}

bool ModrinthPackExportTask::abort()
{
    if (task) {
        task->abort();
        emitAborted();
        return true;
    }
    return false;
}

void ModrinthPackExportTask::collectFiles()
{
    setAbortable(false);
    QCoreApplication::processEvents();

    files.clear();
    if (!MMCZip::collectFileListRecursively(instance->gameRoot(), nullptr, &files, filter)) {
        emitFailed(tr("Could not search for files"));
        return;
    }

    pendingHashes.clear();
    resolvedFiles.clear();

    if (mcInstance) {
        mcInstance->loaderModList()->update();
        connect(mcInstance->loaderModList().get(), &ModFolderModel::updateFinished, this, &ModrinthPackExportTask::collectHashes);
    } else
        collectHashes();
}

void ModrinthPackExportTask::collectHashes()
{
    setStatus(tr("Finding file hashes..."));
    for (const QFileInfo& file : files) {
        QCoreApplication::processEvents();

        const QString relative = gameRoot.relativeFilePath(file.absoluteFilePath());
        // require sensible file types
        if (!std::any_of(PREFIXES.begin(), PREFIXES.end(), [&relative](const QString& prefix) { return relative.startsWith(prefix); }))
            continue;
        if (!std::any_of(FILE_EXTENSIONS.begin(), FILE_EXTENSIONS.end(), [&relative](const QString& extension) {
                return relative.endsWith('.' + extension) || relative.endsWith('.' + extension + ".disabled");
            }))
            continue;

        QCryptographicHash sha512(QCryptographicHash::Algorithm::Sha512);

        QFile openFile(file.absoluteFilePath());
        if (!openFile.open(QFile::ReadOnly)) {
            qWarning() << "Could not open" << file << "for hashing";
            continue;
        }

        const QByteArray data = openFile.readAll();
        if (openFile.error() != QFileDevice::NoError) {
            qWarning() << "Could not read" << file;
            continue;
        }
        sha512.addData(data);

        auto allMods = mcInstance->loaderModList()->allMods();
        if (auto modIter = std::find_if(allMods.begin(), allMods.end(), [&file](Mod* mod) { return mod->fileinfo() == file; });
            modIter != allMods.end()) {
            const Mod* mod = *modIter;
            if (mod->metadata() != nullptr) {
                QUrl& url = mod->metadata()->url;
                // ensure the url is permitted on modrinth.com
                if (!url.isEmpty() && BuildConfig.MODRINTH_MRPACK_HOSTS.contains(url.host())) {
                    qDebug() << "Resolving" << relative << "from index";

                    QCryptographicHash sha1(QCryptographicHash::Algorithm::Sha1);
                    sha1.addData(data);

                    ResolvedFile resolvedFile{ sha1.result().toHex(), sha512.result().toHex(), url.toEncoded(), openFile.size() };
                    resolvedFiles[relative] = resolvedFile;

                    // nice! we've managed to resolve based on local metadata!
                    // no need to enqueue it
                    continue;
                }
            }
        }

        qDebug() << "Enqueueing" << relative << "for Modrinth query";
        pendingHashes[relative] = sha512.result().toHex();
    }

    setAbortable(true);
    makeApiRequest();
}

void ModrinthPackExportTask::makeApiRequest()
{
    if (pendingHashes.isEmpty())
        buildZip();
    else {
        setStatus(tr("Finding versions for hashes..."));
        auto response = std::make_shared<QByteArray>();
        task = api.currentVersions(pendingHashes.values(), "sha512", response);
        connect(task.get(), &NetJob::succeeded, [this, response]() { parseApiResponse(response); });
        connect(task.get(), &NetJob::failed, this, &ModrinthPackExportTask::emitFailed);
        task->start();
    }
}

void ModrinthPackExportTask::parseApiResponse(const std::shared_ptr<QByteArray> response)
{
    task = nullptr;

    try {
        const QJsonDocument doc = Json::requireDocument(*response);

        QMapIterator<QString, QString> iterator(pendingHashes);
        while (iterator.hasNext()) {
            iterator.next();

            const QJsonObject obj = doc[iterator.value()].toObject();
            if (obj.isEmpty())
                continue;

            const QJsonArray files_array = obj["files"].toArray();
            if (auto fileIter = std::find_if(files_array.begin(), files_array.end(),
                                             [&iterator](const QJsonValue& file) { return file["hashes"]["sha512"] == iterator.value(); });
                fileIter != files_array.end()) {
                // map the file to the url
                resolvedFiles[iterator.key()] =
                    ResolvedFile{ fileIter->toObject()["hashes"].toObject()["sha1"].toString(), iterator.value(),
                                  fileIter->toObject()["url"].toString(), fileIter->toObject()["size"].toInt() };
            }
        }
    } catch (const Json::JsonException& e) {
        emitFailed(tr("Failed to parse versions response: %1").arg(e.what()));
        return;
    }
    pendingHashes.clear();
    buildZip();
}

void ModrinthPackExportTask::buildZip()
{
    setStatus(tr("Adding files..."));

    auto zipTask = makeShared<MMCZip::ExportToZipTask>(output, gameRoot, files, "overrides/", true);
    zipTask->addExtraFile("modrinth.index.json", generateIndex());

    zipTask->setExcludeFiles(resolvedFiles.keys());

    auto progressStep = std::make_shared<TaskStepProgress>();
    connect(zipTask.get(), &Task::finished, this, [this, progressStep] {
        progressStep->state = TaskStepState::Succeeded;
        stepProgress(*progressStep);
    });

    connect(zipTask.get(), &Task::succeeded, this, &ModrinthPackExportTask::emitSucceeded);
    connect(zipTask.get(), &Task::aborted, this, &ModrinthPackExportTask::emitAborted);
    connect(zipTask.get(), &Task::failed, this, [this, progressStep](QString reason) {
        progressStep->state = TaskStepState::Failed;
        stepProgress(*progressStep);
        emitFailed(reason);
    });
    connect(zipTask.get(), &Task::stepProgress, this, &ModrinthPackExportTask::propagateStepProgress);

    connect(zipTask.get(), &Task::progress, this, [this, progressStep](qint64 current, qint64 total) {
        progressStep->update(current, total);
        stepProgress(*progressStep);
    });
    connect(zipTask.get(), &Task::status, this, [this, progressStep](QString status) {
        progressStep->status = status;
        stepProgress(*progressStep);
    });
    task.reset(zipTask);
    zipTask->start();
}

QByteArray ModrinthPackExportTask::generateIndex()
{
    QJsonObject out;
    out["formatVersion"] = 1;
    out["game"] = "minecraft";
    out["name"] = name;
    out["versionId"] = version;
    if (!summary.isEmpty())
        out["summary"] = summary;

    if (mcInstance) {
        auto profile = mcInstance->getPackProfile();
        // collect all supported components
        const ComponentPtr minecraft = profile->getComponent("net.minecraft");
        const ComponentPtr quilt = profile->getComponent("org.quiltmc.quilt-loader");
        const ComponentPtr fabric = profile->getComponent("net.fabricmc.fabric-loader");
        const ComponentPtr forge = profile->getComponent("net.minecraftforge");
        const ComponentPtr neoForge = profile->getComponent("net.neoforged");

        // convert all available components to mrpack dependencies
        QJsonObject dependencies;
        if (minecraft != nullptr)
            dependencies["minecraft"] = minecraft->m_version;
        if (quilt != nullptr)
            dependencies["quilt-loader"] = quilt->m_version;
        if (fabric != nullptr)
            dependencies["fabric-loader"] = fabric->m_version;
        if (forge != nullptr)
            dependencies["forge"] = forge->m_version;
        if (neoForge != nullptr)
            dependencies["neoforge"] = neoForge->m_version;

        out["dependencies"] = dependencies;
    }

    QJsonArray filesOut;
    for (auto iterator = resolvedFiles.constBegin(); iterator != resolvedFiles.constEnd(); iterator++) {
        QJsonObject fileOut;

        QString path = iterator.key();
        const ResolvedFile& value = iterator.value();

        if (optionalFiles) {
            // detect disabled mod
            const QFileInfo pathInfo(path);
            if (pathInfo.suffix() == "disabled") {
                // rename it
                path = pathInfo.dir().filePath(pathInfo.completeBaseName());
                // ...and make it optional
                QJsonObject env;
                env["client"] = "optional";
                env["server"] = "optional";
                fileOut["env"] = env;
            }
        }

        fileOut["path"] = path;
        fileOut["downloads"] = QJsonArray{ iterator.value().url };

        QJsonObject hashes;
        hashes["sha1"] = value.sha1;
        hashes["sha512"] = value.sha512;
        fileOut["hashes"] = hashes;

        fileOut["fileSize"] = value.size;
        filesOut << fileOut;
    }
    out["files"] = filesOut;

    return QJsonDocument(out).toJson(QJsonDocument::Compact);
}
