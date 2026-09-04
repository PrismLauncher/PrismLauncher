// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "GetModDependenciesTask.h"

#include <QDebug>
#include <algorithm>
#include <memory>
#include <utility>
#include "Json.h"
#include "QObjectPtr.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/MetadataHandler.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourceFolderModel.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "tasks/SequentialTask.h"
#include "ui/pages/modplatform/ModModel.h"

namespace {

Version mcVersion(MinecraftInstance* inst)
{
    return inst->getPackProfile()->getComponent("net.minecraft")->getVersion();
}

ModPlatform::ModLoaderTypes mcLoaders(MinecraftInstance* inst)
{
    return inst->getPackProfile()->getSupportedModLoaders().value_or(ModPlatform::ModLoaderTypes(0));
}

bool checkDependencies(const std::shared_ptr<GetModDependenciesTask::PackDependency>& sel,
                       const Version& mcVersion,
                       ModPlatform::ModLoaderTypes loaders)
{
    return (sel->pack->versions.isEmpty() || sel->version.mcVersion.contains(mcVersion.toString())) &&
           (!loaders || !sel->version.loaders || sel->version.loaders.testAnyFlags(loaders));
}

// super lax compare (but not fuzzy)
// convert to lowercase
// convert all speratores to whitespace
// simplify sequence of internal whitespace to a single space
// efectivly compare two strings ignoring all separators and case
bool laxCompare(const QString& fsfilename, const QString& metadataFilename, bool excludeDigits = false)
{
    // allowed character seperators
    QList<QChar> allowedSeperators = { '-', '+', '.', '_' };
    if (excludeDigits) {
        allowedSeperators.append({ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' });
    }

    // copy in lowercase
    auto fsName = fsfilename.toLower();
    auto metaName = metadataFilename.toLower();

    // replace all potential allowed seperatores with whitespace
    for (auto sep : allowedSeperators) {
        fsName = fsName.replace(sep, ' ');
        metaName = metaName.replace(sep, ' ');
    }

    // remove extraneous whitespace
    fsName = fsName.simplified();
    metaName = metaName.simplified();

    return fsName.compare(metaName) == 0;
};
}  // namespace

GetModDependenciesTask::GetModDependenciesTask(MinecraftInstance* instance,
                                               ModFolderModel* folder,
                                               QList<std::shared_ptr<PackDependency>> selected)
    : SequentialTask(tr("Get dependencies"))
    , m_selected(std::move(selected))
    , m_version(mcVersion(instance))
    , m_loaderType(mcLoaders(instance))
{
    for (auto* mod : folder->allMods()) {
        m_modsFileNames << mod->fileinfo().fileName();
        if (auto meta = mod->metadata(); meta) {
            m_mods.append(meta);
        }
    }

    for (auto* model : instance->resourceLists()) {
        if (model) {
            for (auto* mod : model->allResources()) {  // only append meta
                if (auto meta = mod->metadata(); meta) {
                    m_mods.append(meta);
                }
            }
        }
    }

    prepare();
}

void GetModDependenciesTask::prepare()
{
    for (const auto& sel : m_selected) {
        if (checkDependencies(sel, m_version, m_loaderType)) {
            for (const auto& dep : getDependenciesForVersion(sel->version, sel->pack->provider)) {
                addTask(prepareDependencyTask(dep, sel->pack->provider, 20));
            }
        }
    }
}

ModPlatform::Dependency GetModDependenciesTask::getOverride(const ModPlatform::Dependency& dep,
                                                            const ModPlatform::ResourceProvider providerName)
{
    if (auto isQuilt = (m_loaderType & ModPlatform::Quilt) != 0U; isQuilt || (m_loaderType & ModPlatform::Fabric) != 0U) {
        auto overide = ModPlatform::getOverrideDeps();
        auto isOverrideForProvider = [dep, providerName, isQuilt](const auto& o) {
            return o.provider == providerName && dep.addonId == (isQuilt ? o.fabric : o.quilt);
        };
        auto over = std::ranges::find_if(overide, isOverrideForProvider);
        if (over != overide.cend()) {
            return { .addonId = isQuilt ? over->quilt : over->fabric, .type = dep.type, .version = "" };
        }
    }
    return dep;
}

QList<ModPlatform::Dependency> GetModDependenciesTask::getDependenciesForVersion(const ModPlatform::IndexedVersion& version,
                                                                                 const ModPlatform::ResourceProvider providerName)
{
    QList<ModPlatform::Dependency> cDependencies;
    for (auto verDep : version.dependencies) {
        if (verDep.type != ModPlatform::DependencyType::REQUIRED) {
            continue;
        }
        verDep = getOverride(verDep, providerName);
        auto isOnlyVersion = providerName == ModPlatform::ResourceProvider::MODRINTH && verDep.addonId.toString().isEmpty();
        auto isDuplicateDep = [&verDep, isOnlyVersion](const ModPlatform::Dependency& i) {
            return isOnlyVersion ? i.version == verDep.version : i.addonId == verDep.addonId;
        };
        if (std::ranges::any_of(cDependencies, isDuplicateDep)) {
            continue;  // check the current dependency list
        }

        auto isKnownDependency = [&verDep, providerName, isOnlyVersion](const std::shared_ptr<PackDependency>& i) {
            return i->pack->provider == providerName &&
                   (isOnlyVersion ? i->version.version == verDep.version : i->pack->addonId == verDep.addonId);
        };
        if (std::ranges::any_of(m_selected, isKnownDependency)) {
            continue;  // check the selected versions
        }

        auto isInstalledMod = [&verDep, providerName, isOnlyVersion](const std::shared_ptr<Metadata::ModStruct>& i) {
            return i->provider == providerName && (isOnlyVersion ? i->fileId == verDep.version : i->projectId == verDep.addonId);
        };
        if (std::ranges::any_of(m_mods, isInstalledMod)) {
            continue;  // check the existing mods
        }

        if (std::ranges::any_of(m_packDependencies, isKnownDependency)) {  // check loaded dependencies
            continue;
        }

        cDependencies.append(verDep);
    }
    return cDependencies;
}

Task::Ptr GetModDependenciesTask::getProjectInfoTask(const std::shared_ptr<PackDependency>& pDep)
{
    auto provider = pDep->pack->provider;
    auto [info, responseInfo] = getAPI(provider)->getProject(pDep->pack->addonId.toString());
    connect(info.get(), &NetJob::succeeded, this, [this, responseInfo, provider, pDep] {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(*responseInfo, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            removePack(pDep->pack->addonId);
            qWarning() << "Error while parsing JSON response for mod info at" << parseError.offset << "reason:" << parseError.errorString();
            qDebug() << *responseInfo;
            return;
        }
        try {
            auto obj = provider == ModPlatform::ResourceProvider::FLAME ? Json::requireObject(Json::requireObject(doc), "data")
                                                                        : Json::requireObject(doc);

            getAPI(provider)->loadIndexedPack(*pDep->pack, obj);
        } catch (const JSONValidationError& e) {
            removePack(pDep->pack->addonId);
            qDebug() << doc;
            qWarning() << "Error while reading mod info:" << e.cause();
        }
    });
    QObject::connect(info.get(), &NetJob::failed, this, [this, info, pDep] {
        removePack(pDep->pack->addonId);
        m_failed.remove(info.get());
    });
    return info;
}

Task::Ptr GetModDependenciesTask::prepareDependencyTask(const ModPlatform::Dependency& dep,
                                                        const ModPlatform::ResourceProvider providerName,
                                                        int level)
{
    auto pDep = std::make_shared<PackDependency>();
    pDep->dependency = dep;
    pDep->pack = std::make_shared<ModPlatform::IndexedPack>();
    pDep->pack->addonId = dep.addonId;
    pDep->pack->provider = providerName;

    m_packDependencies.append(pDep);

    auto provider = providerName;

    auto tasks = makeShared<SequentialTask>(
        QString("DependencyInfo: %1").arg(dep.addonId.toString().isEmpty() ? dep.version : dep.addonId.toString()));

    if (!dep.addonId.toString().isEmpty()) {
        tasks->addTask(getProjectInfoTask(pDep));
    }

    ResourceAPI::DependencySearchArgs args = {
        .dependency = dep, .mcVersion = m_version, .loader = m_loaderType, .includeChangelog = true
    };
    ResourceAPI::Callback<ModPlatform::IndexedVersion> callbacks;
    callbacks.onFail = [](const QString& reason, int) {
        qCritical() << tr("A network error occurred. Could not load project dependencies:%1").arg(reason);
    };
    callbacks.onSucceed = [dep, provider, pDep, level, this](auto& pack) {
        pDep->version = pack;
        if (!pDep->version.addonId.isValid()) {
            if (m_loaderType & ModPlatform::Quilt) {  // falback for quilt
                auto overide = ModPlatform::getOverrideDeps();
                auto isQuiltFallback = [dep, provider](const auto& o) { return o.provider == provider && dep.addonId == o.quilt; };
                auto over = std::ranges::find_if(overide, isQuiltFallback);
                if (over != overide.cend()) {
                    removePack(dep.addonId);
                    addTask(prepareDependencyTask({ .addonId = over->fabric, .type = dep.type, .version = "" }, provider, level));
                    return;
                }
            }
            removePack(dep.addonId);
            return;
        }
        pDep->version.isCurrentlySelected = true;
        pDep->pack->versions = { pDep->version };
        pDep->pack->versionsLoaded = true;

        if (level == 0) {
            removePack(dep.addonId);
            qWarning() << "Dependency cycle exceeded";
            return;
        }
        if (dep.addonId.toString().isEmpty() && !pDep->version.addonId.toString().isEmpty()) {
            pDep->pack->addonId = pDep->version.addonId;
            auto overrideDep = getOverride({ .addonId = pDep->version.addonId, .type = pDep->dependency.type, .version = "" }, provider);
            if (overrideDep.addonId != pDep->version.addonId) {
                removePack(pDep->version.addonId);
                addTask(prepareDependencyTask(overrideDep, provider, level));
            } else {
                addTask(getProjectInfoTask(pDep));
            }
        }
        if (isLocalyInstalled(pDep)) {
            removePack(pDep->version.addonId);
            return;
        }
        for (const auto& dependency : getDependenciesForVersion(pDep->version, provider)) {
            addTask(prepareDependencyTask(dependency, provider, level - 1));
        }
    };

    auto version = getAPI(provider)->getDependencyVersion(args, callbacks);
    QObject::connect(version.get(), &NetJob::failed, this, [this, version, pDep] {
        removePack(pDep->pack->addonId);
        m_failed.remove(version.get());
    });
    tasks->addTask(version);
    return tasks;
}

void GetModDependenciesTask::removePack(const QVariant& addonId)
{
    auto pred = [addonId](const std::shared_ptr<PackDependency>& v) { return v->pack->addonId == addonId; };
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    m_packDependencies.removeIf(pred);
#else
    for (auto it = m_pack_dependencies.begin(); it != m_pack_dependencies.end();)
        if (pred(*it))
            it = m_pack_dependencies.erase(it);
        else
            ++it;
#endif
}

auto GetModDependenciesTask::getExtraInfo() -> QHash<QString, PackDependencyExtraInfo>
{
    QHash<QString, PackDependencyExtraInfo> rby;
    auto fullList = m_selected + m_packDependencies;
    for (auto& mod : fullList) {
        auto addonId = mod->pack->addonId;
        auto provider = mod->pack->provider;
        auto version = mod->version.fileId;
        auto reqNames = QStringList();
        auto reqIds = QStringList();
        for (auto& smod : fullList) {
            if (provider != smod->pack->provider) {
                continue;
            }
            auto deps = smod->version.dependencies;
            auto isRequiredByOther = [addonId, provider, version](const ModPlatform::Dependency& d) {
                return d.type == ModPlatform::DependencyType::REQUIRED &&
                       (provider == ModPlatform::ResourceProvider::MODRINTH && d.addonId.toString().isEmpty() ? version == d.version
                                                                                                              : d.addonId == addonId);
            };
            if (std::ranges::any_of(deps, isRequiredByOther)) {
                reqNames.append(smod->pack->name);
                reqIds.append(smod->version.fileId.toString());
            }
        }
        rby[addonId.toString()] = { .maybeInstalled = maybeInstalled(mod), .requiredByNames = reqNames, .requiredByIds = reqIds };
    }
    return rby;
}

bool GetModDependenciesTask::isLocalyInstalled(const std::shared_ptr<PackDependency>& pDep)
{
    auto isAlreadySelected = [pDep](const std::shared_ptr<PackDependency>& i) {
        return !i->version.fileName.isEmpty() && laxCompare(i->version.fileName, pDep->version.fileName);
    };
    auto isExistingFile = [pDep](const QString& i) { return !i.isEmpty() && laxCompare(i, pDep->version.fileName); };
    auto isKnownFile = [pDep](const std::shared_ptr<PackDependency>& i) {
        return pDep->pack->addonId != i->pack->addonId && !i->version.fileName.isEmpty() &&
               laxCompare(pDep->version.fileName, i->version.fileName);
    };

    return pDep->version.fileName.isEmpty() || std::ranges::any_of(m_selected, isAlreadySelected) ||  // check the selected versions
           std::ranges::any_of(m_modsFileNames, isExistingFile) ||                                    // check the existing mods
           std::ranges::any_of(m_packDependencies, isKnownFile);                                      // check loaded dependencies
}

bool GetModDependenciesTask::maybeInstalled(const std::shared_ptr<PackDependency>& pDep)
{
    auto isExistingFileLoose = [pDep](const QString& i) { return !i.isEmpty() && laxCompare(i, pDep->version.fileName, true); };
    return std::ranges::any_of(m_modsFileNames, isExistingFileLoose);  // check the existing mods
}
