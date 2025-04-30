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
#include "Application.h"
#include "QObjectPtr.h"
#include "api/structures/Arguments.h"
#include "api/structures/Project.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/MetadataHandler.h"
#include "tasks/SequentialTask.h"
#include "ui/pages/modplatform/ModModel.h"

static Version mcVersion(BaseInstance* inst)
{
    return static_cast<MinecraftInstance*>(inst)->getPackProfile()->getComponent("net.minecraft")->getVersion();
}

static Platform::ModLoaders mcLoaders(BaseInstance* inst)
{
    return static_cast<MinecraftInstance*>(inst)->getPackProfile()->getSupportedModLoaders().value();
}

static bool checkDependencies(std::shared_ptr<GetModDependenciesTask::PackDependency> sel, Version mcVersion, Platform::ModLoaders loaders)
{
    return (sel->pack->versions.isEmpty() || sel->version.mcVersion.contains(mcVersion.toString())) &&
           (!loaders || !sel->version.loaders || sel->version.loaders & loaders);
}

GetModDependenciesTask::GetModDependenciesTask(BaseInstance* instance,
                                               ModFolderModel* folder,
                                               QList<std::shared_ptr<PackDependency>> selected)
    : SequentialTask(tr("Get dependencies")), m_selected(selected), m_version(mcVersion(instance)), m_loaderType(mcLoaders(instance))
{
    for (auto mod : folder->allMods()) {
        m_mods_file_names << mod->fileinfo().fileName();
        if (auto meta = mod->metadata(); meta)
            m_mods.append(meta);
    }
    prepare();
}

void GetModDependenciesTask::prepare()
{
    for (auto sel : m_selected) {
        if (checkDependencies(sel, m_version, m_loaderType))
            for (auto dep : getDependenciesForVersion(sel->version, sel->pack->provider)) {
                addTask(prepareDependencyTask(dep, sel->pack->provider, 20));
            }
    }
}

Platform::Dependency GetModDependenciesTask::getOverride(const Platform::Dependency& dep, const Platform::Provider providerName)
{
    if (auto isQuilt = m_loaderType & Platform::ModLoader::Quilt; isQuilt || m_loaderType & Platform::ModLoader::Fabric) {
        auto overide = Platform::getOverrideDeps();
        auto over = std::find_if(overide.cbegin(), overide.cend(), [dep, providerName, isQuilt](auto o) {
            return o.provider == providerName && dep.projectId == (isQuilt ? o.fabric : o.quilt);
        });
        if (over != overide.cend()) {
            return { isQuilt ? over->quilt : over->fabric, dep.type };
        }
    }
    return dep;
}

QList<Platform::Dependency> GetModDependenciesTask::getDependenciesForVersion(const Platform::Version& version,
                                                                              const Platform::Provider providerName)
{
    QList<Platform::Dependency> c_dependencies;
    for (auto ver_dep : version.dependencies) {
        if (ver_dep.type != Platform::DependencyType::REQUIRED)
            continue;
        ver_dep = getOverride(ver_dep, providerName);
        auto isOnlyVersion = providerName == Platform::Provider::MODRINTH && ver_dep.projectId.toString().isEmpty();
        if (auto dep = std::find_if(c_dependencies.begin(), c_dependencies.end(),
                                    [&ver_dep, isOnlyVersion](const Platform::Dependency& i) {
                                        return isOnlyVersion ? i.version == ver_dep.version : i.projectId == ver_dep.projectId;
                                    });
            dep != c_dependencies.end())
            continue;  // check the current dependency list

        if (auto dep =
                std::find_if(m_selected.begin(), m_selected.end(),
                             [&ver_dep, providerName, isOnlyVersion](std::shared_ptr<PackDependency> i) {
                                 return i->pack->provider == providerName &&
                                        (isOnlyVersion ? i->version.version == ver_dep.version : i->pack->projectId == ver_dep.projectId);
                             });
            dep != m_selected.end())
            continue;  // check the selected versions

        if (auto dep = std::find_if(m_mods.begin(), m_mods.end(),
                                    [&ver_dep, providerName, isOnlyVersion](std::shared_ptr<Metadata::ModStruct> i) {
                                        return i->provider == providerName &&
                                               (isOnlyVersion ? i->file_id == ver_dep.version : i->project_id == ver_dep.projectId);
                                    });
            dep != m_mods.end())
            continue;  // check the existing mods

        if (auto dep =
                std::find_if(m_pack_dependencies.begin(), m_pack_dependencies.end(),
                             [&ver_dep, providerName, isOnlyVersion](std::shared_ptr<PackDependency> i) {
                                 return i->pack->provider == providerName &&
                                        (isOnlyVersion ? i->version.version == ver_dep.projectId : i->pack->projectId == ver_dep.projectId);
                             });
            dep != m_pack_dependencies.end())  // check loaded dependencies
            continue;

        c_dependencies.append(ver_dep);
    }
    return c_dependencies;
}

Task::Ptr GetModDependenciesTask::getProjectInfoTask(std::shared_ptr<PackDependency> pDep)
{
    auto provider = pDep->pack->provider;
    auto job = makeShared<NetJob>(QString("%1::GetProject").arg(pDep->pack->projectId.toString()), APPLICATION->network());

    auto info = API::ProviderAPI::get(provider)->makeGetProjectRequest(pDep->pack->projectId.toString(), pDep->pack);
    job->addNetAction(info);
    QObject::connect(job.get(), &NetJob::failed, [this, pDep] { removePack(pDep->pack->projectId); });
    return job;
}

Task::Ptr GetModDependenciesTask::prepareDependencyTask(const Platform::Dependency& dep, const Platform::Provider providerName, int level)
{
    auto pDep = std::make_shared<PackDependency>();
    pDep->dependency = dep;
    pDep->pack = std::make_shared<Platform::Project>();
    pDep->pack->projectId = dep.projectId;
    pDep->pack->provider = providerName;

    m_pack_dependencies.append(pDep);

    auto provider = providerName;

    auto tasks = makeShared<SequentialTask>(
        QString("DependencyInfo: %1").arg(dep.projectId.toString().isEmpty() ? dep.version : dep.projectId.toString()));

    if (!dep.projectId.toString().isEmpty()) {
        tasks->addTask(getProjectInfoTask(pDep));
    }

    API::DependencySearchArgs args = { dep, m_loaderType, m_version };
    API::Callback<Platform::Version> callbacks;
    callbacks.on_fail = [](QString reason, int) {
        qCritical() << tr("A network error occurred. Could not load project dependencies:%1").arg(reason);
    };
    callbacks.on_succeed = [dep, provider, pDep, level, this](auto& pack) {
        pDep->version = pack;
        if (!pDep->version.projectId.isValid()) {
            if (m_loaderType & Platform::ModLoader::Quilt) {  // falback for quilt
                auto overide = Platform::getOverrideDeps();
                auto over = std::find_if(overide.cbegin(), overide.cend(),
                                         [&dep, provider](const auto& o) { return o.provider == provider && dep.projectId == o.quilt; });
                if (over != overide.cend()) {
                    removePack(dep.projectId);
                    addTask(prepareDependencyTask({ over->fabric, dep.type }, provider, level));
                    return;
                }
            }
            removePack(dep.projectId);
            return;
        }
        pDep->version.is_currently_selected = true;
        pDep->pack->versions = { pDep->version };
        pDep->pack->versionsLoaded = true;

        if (level == 0) {
            removePack(dep.projectId);
            qWarning() << "Dependency cycle exceeded";
            return;
        }
        if (dep.projectId.toString().isEmpty() && !pDep->version.projectId.toString().isEmpty()) {
            pDep->pack->projectId = pDep->version.projectId;
            auto dep_ = getOverride({ pDep->version.projectId, pDep->dependency.type }, provider);
            if (dep_.projectId != pDep->version.projectId) {
                removePack(pDep->version.projectId);
                addTask(prepareDependencyTask(dep_, provider, level));
            } else {
                addTask(getProjectInfoTask(pDep));
            }
        }
        if (isLocalyInstalled(pDep)) {
            removePack(pDep->version.projectId);
            return;
        }
        for (auto dep_ : getDependenciesForVersion(pDep->version, provider)) {
            addTask(prepareDependencyTask(dep_, provider, level - 1));
        }
    };

    auto version = API::ProviderAPI::get(provider)->getDependencyVersion(std::move(args), std::move(callbacks));
    tasks->addTask(version);
    return tasks;
}

void GetModDependenciesTask::removePack(const QVariant& addonId)
{
    auto pred = [addonId](const std::shared_ptr<PackDependency>& v) { return v->pack->projectId == addonId; };
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    m_pack_dependencies.removeIf(pred);
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
    auto fullList = m_selected + m_pack_dependencies;
    for (auto& mod : fullList) {
        auto addonId = mod->pack->projectId;
        auto provider = mod->pack->provider;
        auto version = mod->version.fileId;
        auto req = QStringList();
        for (auto& smod : fullList) {
            if (provider != smod->pack->provider)
                continue;
            auto deps = smod->version.dependencies;
            if (auto dep = std::find_if(deps.begin(), deps.end(),
                                        [addonId, provider, version](const Platform::Dependency& d) {
                                            return d.type == Platform::DependencyType::REQUIRED &&
                                                   (provider == Platform::Provider::MODRINTH && d.projectId.toString().isEmpty()
                                                        ? version == d.version
                                                        : d.projectId == addonId);
                                        });
                dep != deps.end()) {
                req.append(smod->pack->name);
            }
        }
        rby[addonId.toString()] = { maybeInstalled(mod), req };
    }
    return rby;
}

// super lax compare (but not fuzzy)
// convert to lowercase
// convert all speratores to whitespace
// simplify sequence of internal whitespace to a single space
// efectivly compare two strings ignoring all separators and case
auto laxCompare = [](QString fsfilename, QString metadataFilename, bool excludeDigits = false) {
    // allowed character seperators
    QList<QChar> allowedSeperators = { '-', '+', '.', '_' };
    if (excludeDigits)
        allowedSeperators.append({ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' });

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

bool GetModDependenciesTask::isLocalyInstalled(std::shared_ptr<PackDependency> pDep)
{
    return pDep->version.fileName.isEmpty() ||

           std::find_if(m_selected.begin(), m_selected.end(),
                        [pDep](std::shared_ptr<PackDependency> i) {
                            return !i->version.fileName.isEmpty() && laxCompare(i->version.fileName, pDep->version.fileName);
                        }) != m_selected.end() ||  // check the selected versions

           std::find_if(m_mods_file_names.begin(), m_mods_file_names.end(),
                        [pDep](QString i) { return !i.isEmpty() && laxCompare(i, pDep->version.fileName); }) !=
               m_mods_file_names.end() ||  // check the existing mods

           std::find_if(m_pack_dependencies.begin(), m_pack_dependencies.end(), [pDep](std::shared_ptr<PackDependency> i) {
               return pDep->pack->projectId != i->pack->projectId && !i->version.fileName.isEmpty() &&
                      laxCompare(pDep->version.fileName, i->version.fileName);
           }) != m_pack_dependencies.end();  // check loaded dependencies
}

bool GetModDependenciesTask::maybeInstalled(std::shared_ptr<PackDependency> pDep)
{
    return std::find_if(m_mods_file_names.begin(), m_mods_file_names.end(), [pDep](QString i) {
               return !i.isEmpty() && laxCompare(i, pDep->version.fileName, true);
           }) != m_mods_file_names.end();  // check the existing mods
}
