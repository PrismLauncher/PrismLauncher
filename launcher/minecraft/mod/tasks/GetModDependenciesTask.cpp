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
#include "Json.h"
#include "QObjectPtr.h"
#include "minecraft/mod/MetadataHandler.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "tasks/ConcurrentTask.h"
#include "tasks/SequentialTask.h"
#include "ui/pages/modplatform/ModModel.h"
#include "ui/pages/modplatform/flame/FlameResourceModels.h"
#include "ui/pages/modplatform/modrinth/ModrinthResourceModels.h"

static Version mcVersions(BaseInstance* inst)
{
    return static_cast<MinecraftInstance*>(inst)->getPackProfile()->getComponent("net.minecraft")->getVersion();
}

static ResourceAPI::ModLoaderTypes mcLoaders(BaseInstance* inst)
{
    return static_cast<MinecraftInstance*>(inst)->getPackProfile()->getModLoaders().value();
}

GetModDependenciesTask::GetModDependenciesTask(QObject* parent,
                                               BaseInstance* instance,
                                               ModFolderModel* folder,
                                               QList<std::shared_ptr<PackDependency>> selected)
    : SequentialTask(parent, "Get dependencies")
    , m_selected(selected)
    , m_flame_provider{ ModPlatform::ResourceProvider::FLAME, std::make_shared<ResourceDownload::FlameModModel>(*instance),
                        std::make_shared<FlameAPI>() }
    , m_modrinth_provider{ ModPlatform::ResourceProvider::MODRINTH, std::make_shared<ResourceDownload::ModrinthModModel>(*instance),
                           std::make_shared<ModrinthAPI>() }
    , m_version(mcVersions(instance))
    , m_loaderType(mcLoaders(instance))
{
    for (auto mod : folder->allMods())
        m_mods.append(mod->metadata());
    prepare();
};

void GetModDependenciesTask::prepare()
{
    for (auto sel : m_selected) {
        for (auto dep : getDependenciesForVersion(sel->version, sel->pack.provider)) {
            addTask(prepareDependencyTask(dep, sel->pack.provider, sel->pack.addonId, 20));
        }
    }
}

QList<ModPlatform::Dependency> GetModDependenciesTask::getDependenciesForVersion(const ModPlatform::IndexedVersion& version,
                                                                                 const ModPlatform::ResourceProvider providerName)
{
    auto c_dependencies = QList<ModPlatform::Dependency>();
    for (auto ver_dep : version.dependencies) {
        if (ver_dep.type == ModPlatform::DependencyType::REQUIRED) {
            if (auto dep = std::find_if(c_dependencies.begin(), c_dependencies.end(),
                                        [&ver_dep](const ModPlatform::Dependency& i) { return i.addonId == ver_dep.addonId; });
                dep == c_dependencies.end()) {  // check the current dependency list
                if (auto dep = std::find_if(m_selected.begin(), m_selected.end(),
                                            [&ver_dep, providerName](std::shared_ptr<PackDependency> i) {
                                                return i->pack.addonId == ver_dep.addonId && i->pack.provider == providerName;
                                            });
                    dep == m_selected.end()) {  // check the selected versions
                    if (auto dep = std::find_if(m_mods.begin(), m_mods.end(),
                                                [&ver_dep, providerName](std::shared_ptr<Metadata::ModStruct> i) {
                                                    return i->project_id == ver_dep.addonId && i->provider == providerName;
                                                });
                        dep == m_mods.end()) {  // check the existing mods
                        if (auto dep = std::find_if(m_pack_dependencies.begin(), m_pack_dependencies.end(),
                                                    [&ver_dep, providerName](std::shared_ptr<PackDependency> i) {
                                                        return i->pack.addonId == ver_dep.addonId && i->pack.provider == providerName;
                                                    });
                            dep == m_pack_dependencies.end()) {  // check loaded dependencies
                            c_dependencies.append(ver_dep);
                        } else {  // already there just append the required_by
                            dep->get()->version.required_by.append(version.addonId);
                        }
                    }
                }
            }
        }
    }
    return c_dependencies;
};

Task::Ptr GetModDependenciesTask::prepareDependencyTask(const ModPlatform::Dependency& dep,
                                                        const ModPlatform::ResourceProvider providerName,
                                                        QVariant required_by,
                                                        int level)
{
    auto pDep = std::make_shared<PackDependency>();
    pDep->dependency = dep;
    pDep->pack = { dep.addonId, providerName };
    pDep->version.required_by.append(required_by);
    m_pack_dependencies.append(pDep);
    auto provider = providerName == m_flame_provider.name ? m_flame_provider : m_modrinth_provider;

    auto tasks = makeShared<SequentialTask>(this, QString("DependencyInfo: %1").arg(dep.addonId.toString()));

    auto responseInfo = new QByteArray();
    auto info = provider.api->getProject(dep.addonId.toString(), responseInfo);
    QObject::connect(info.get(), &NetJob::succeeded, [responseInfo, provider, pDep] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*responseInfo, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for mod info at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *responseInfo;
            return;
        }
        try {
            auto obj = provider.name == ModPlatform::ResourceProvider::FLAME ? Json::requireObject(Json::requireObject(doc), "data")
                                                                             : Json::requireObject(doc);
            provider.mod->loadIndexedPack(pDep->pack, obj);
        } catch (const JSONValidationError& e) {
            qDebug() << doc;
            qWarning() << "Error while reading mod info: " << e.cause();
        }
    });
    tasks->addTask(info);

    ResourceAPI::DependencySearchArgs args = { dep, m_version, m_loaderType };
    ResourceAPI::DependencySearchCallbacks callbacks;

    callbacks.on_succeed = [dep, provider, pDep, level, required_by, this](auto& doc, auto& pack) {
        try {
            QJsonArray arr;
            if (dep.version.length() != 0 && doc.isObject()) {
                arr.append(doc.object());
            } else {
                arr = doc.isObject() ? Json::ensureArray(doc.object(), "data") : doc.array();
            }
            auto required_by = pDep->version.required_by;
            pDep->version = provider.mod->loadDependencyVersions(dep, arr);
            if (!pDep->version.addonId.isValid()) {
                qWarning() << "Error while reading mod version empty ";
                qDebug() << doc;
                return;
            }
            pDep->version.is_currently_selected = true;
            pDep->version.required_by = required_by;
            pDep->pack.versions = { pDep->version };
            pDep->pack.versionsLoaded = true;

        } catch (const JSONValidationError& e) {
            qDebug() << doc;
            qWarning() << "Error while reading mod version: " << e.cause();
            return;
        }
        if (level == 0) {
            qWarning() << "Dependency cycle exeeded";
            return;
        }
        for (auto dep : getDependenciesForVersion(pDep->version, provider.name)) {
            addTask(prepareDependencyTask(dep, provider.name, pDep->pack.addonId, level - 1));
        }
    };

    auto version = provider.api->getDependencyVersion(std::move(args), std::move(callbacks));
    tasks->addTask(version);
    return tasks;
};
