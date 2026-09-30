// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
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

#include "ModApi.h"

#include <QJsonArray>

#include "Application.h"
#include "ResourceDownloadTask.h"
#include "Version.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "settings/SettingsObject.h"

#include "ApiRouter.h"
#include "ApiUtils.h"
#include "ResourceApi.h"
#include "TaskTracker.h"

namespace api {

namespace {

constexpr std::pair<const char*, ModPlatform::ResourceProvider> Providers[] = {
    { "modrinth", ModPlatform::ResourceProvider::MODRINTH },
    { "curseforge", ModPlatform::ResourceProvider::FLAME },
};

constexpr std::pair<const char*, ModPlatform::ResourceType> Kinds[] = {
    { "mods", ModPlatform::ResourceType::Mod },
    { "resourcepacks", ModPlatform::ResourceType::ResourcePack },
    { "shaderpacks", ModPlatform::ResourceType::ShaderPack },
    { "texturepacks", ModPlatform::ResourceType::TexturePack },
};

const ResourceAPI& apiFor(ModPlatform::ResourceProvider provider)
{
    if (provider == ModPlatform::ResourceProvider::FLAME) {
        if (!APPLICATION->capabilities().testFlag(Application::SupportsFlame)) {
            throw ApiError::unsupported(QObject::tr("This build has no CurseForge API key"));
        }
        return FlameAPI::get();
    }
    return ModrinthAPI::get();
}

const char* providerName(ModPlatform::ResourceProvider provider)
{
    return provider == ModPlatform::ResourceProvider::FLAME ? "curseforge" : "modrinth";
}

QJsonObject serializePack(const ModPlatform::IndexedPack& pack)
{
    QJsonArray authors;
    for (const auto& a : pack.authors) {
        authors.append(a.name);
    }
    return {
        { "provider", providerName(pack.provider) },
        { "id", pack.addonId.toString() },
        { "slug", pack.slug },
        { "name", pack.name },
        { "description", pack.description },
        { "authors", authors },
        { "iconUrl", pack.logoUrl.startsWith("https://") ? pack.logoUrl : QString() },
        { "websiteUrl", pack.websiteUrl },
    };
}

struct InstanceFilter {
    QString mcVersion;
    std::optional<ModPlatform::ModLoaderTypes> loaders;
};

InstanceFilter filterFor(MinecraftInstance* instance, ModPlatform::ResourceType type)
{
    InstanceFilter filter;
    auto* profile = loadedPackProfile(instance);
    if (profile) {
        filter.mcVersion = profile->getComponentVersion("net.minecraft");
        if (type == ModPlatform::ResourceType::Mod) {
            filter.loaders = profile->getSupportedModLoaders();
        }
    }
    return filter;
}

bool isCompatible(const ModPlatform::IndexedVersion& v, const InstanceFilter& filter, ModPlatform::ResourceType type)
{
    if (!filter.mcVersion.isEmpty() && !v.mcVersion.isEmpty() && !v.mcVersion.contains(filter.mcVersion)) {
        return false;
    }
    if (type == ModPlatform::ResourceType::Mod && filter.loaders && v.loaders != 0U && (v.loaders & *filter.loaders) == 0U) {
        return false;
    }
    return true;
}

QJsonObject serializeVersion(const ModPlatform::IndexedVersion& v, bool compatible)
{
    QJsonArray loaders;
    for (auto loader : ModPlatform::modLoaderTypesToList(v.loaders)) {
        loaders.append(ModPlatform::getModLoaderAsString(loader));
    }
    return {
        { "id", v.fileId.toString() },
        { "name", v.version },
        { "versionNumber", v.versionNumber },
        { "type", v.versionType.isValid() ? v.versionType.toString() : QString() },
        { "gameVersions", QJsonArray::fromStringList(v.mcVersion) },
        { "loaders", loaders },
        { "date", v.date },
        { "fileName", v.fileName },
        { "compatible", compatible },
    };
}

}  // namespace

ModApi::ModApi(ApiRouter* router, TaskTracker* tasks, QObject* parent) : QObject(parent), m_router(router), m_tasks(tasks)
{
    registerMethods();
}

QString ModApi::cacheKey(ModPlatform::ResourceProvider provider, const QString& projectId)
{
    return QStringLiteral("%1:%2").arg(QString::fromLatin1(providerName(provider)), projectId);
}

void ModApi::runRequest(const Task::Ptr& task)
{
    m_requests.insert(task);
    connect(task.get(), &Task::finished, this, [this, task] { QMetaObject::invokeMethod(this, [this, task] { m_requests.remove(task); }, Qt::QueuedConnection); },
            Qt::SingleShotConnection);
    task->start();
}

void ModApi::registerMethods()
{
    m_router->add("mods.search", [this](const QJsonObject& p, const ApiReply& reply) {
        auto* instance = requireInstance(p, "instanceId");
        const auto type = params::requireEnum(p, "kind", Kinds);
        const auto provider = params::requireEnum(p, "provider", Providers);
        const auto& api = apiFor(provider);
        const auto query = params::requireString(p, "query", 256).trimmed();
        const auto offset = static_cast<int>(params::optionalInt(p, "offset", 0, 0, 100000));
        const auto sortId = params::optionalString(p, "sort", 64);

        const auto methods = api.getSortingMethods();
        std::optional<ResourceAPI::SortingMethod> sorting;
        QJsonArray sortingJson;
        for (const auto& m : methods) {
            sortingJson.append(QJsonObject{ { "id", m.name }, { "name", m.readableName } });
            if (sortId && m.name == *sortId) {
                sorting = m;
            }
        }
        if (!sorting && !methods.isEmpty()) {
            sorting = methods.first();
        }

        const auto filter = filterFor(instance, type);
        ResourceAPI::SearchArgs args{ .type = type, .offset = offset, .search = query, .sorting = sorting };
        if (type == ModPlatform::ResourceType::Mod) {
            args.loaders = filter.loaders;
            if (!filter.mcVersion.isEmpty()) {
                args.versions = std::vector<Version>{ Version(filter.mcVersion) };
            }
        }

        ResourceAPI::Callback<QList<ModPlatform::IndexedPack::Ptr>> callbacks;
        callbacks.onSucceed = [this, reply, offset, sortingJson](QList<ModPlatform::IndexedPack::Ptr>& packs) {
            QJsonArray projects;
            for (const auto& pack : packs) {
                m_packs.insert(cacheKey(pack->provider, pack->addonId.toString()), pack);
                projects.append(serializePack(*pack));
            }
            reply.resolve(QJsonObject{ { "projects", projects }, { "offset", offset }, { "sortingMethods", sortingJson } });
        };
        callbacks.onFail = [reply](const QString& reason, int) { reply.reject(ApiError("NETWORK_ERROR", reason)); };
        callbacks.onAbort = [reply] { reply.reject(ApiError::cancelled()); };

        auto task = api.searchProjects(args, callbacks);
        if (!task) {
            throw ApiError::internal(tr("Could not create the search request"));
        }
        runRequest(task);
    });

    m_router->add("mods.versions", [this](const QJsonObject& p, const ApiReply& reply) {
        auto* instance = requireInstance(p, "instanceId");
        const auto type = params::requireEnum(p, "kind", Kinds);
        const auto provider = params::requireEnum(p, "provider", Providers);
        const auto& api = apiFor(provider);
        const auto projectId = params::requireNonEmpty(p, "projectId", 128);
        const auto key = cacheKey(provider, projectId);
        const auto pack = m_packs.value(key);
        if (!pack) {
            throw ApiError::notFound(tr("Unknown project %1; search for it first").arg(projectId));
        }

        const auto filter = filterFor(instance, type);
        // Request every version and flag compatibility here, so the UI can offer "show all".
        ResourceAPI::VersionSearchArgs args{ .pack = pack, .mcVersions = {}, .loaders = {}, .resourceType = type };
        ResourceAPI::Callback<QVector<ModPlatform::IndexedVersion>> callbacks;
        callbacks.onSucceed = [this, reply, key, filter, type](QVector<ModPlatform::IndexedVersion>& versions) {
            m_versions.insert(key, versions);
            QJsonArray out;
            for (const auto& v : versions) {
                out.append(serializeVersion(v, isCompatible(v, filter, type)));
            }
            reply.resolve(out);
        };
        callbacks.onFail = [reply](const QString& reason, int) { reply.reject(ApiError("NETWORK_ERROR", reason)); };
        callbacks.onAbort = [reply] { reply.reject(ApiError::cancelled()); };

        auto task = api.getProjectVersions(args, callbacks);
        if (!task) {
            throw ApiError::internal(tr("Could not create the versions request"));
        }
        runRequest(task);
    });

    m_router->addSync("mods.install", [this](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        const auto kind = params::requireNonEmpty(p, "kind", 32);
        params::requireEnum(p, "kind", Kinds);
        const auto provider = params::requireEnum(p, "provider", Providers);
        const auto projectId = params::requireNonEmpty(p, "projectId", 128);
        const auto versionId = params::requireNonEmpty(p, "versionId", 128);
        const auto key = cacheKey(provider, projectId);
        const auto pack = m_packs.value(key);
        const auto versions = m_versions.value(key);
        const auto version = std::find_if(versions.begin(), versions.end(), [&](const auto& v) { return v.fileId.toString() == versionId; });
        if (!pack || version == versions.end()) {
            throw ApiError::notFound(tr("Unknown version %1 of %2; list the versions first").arg(versionId, projectId));
        }
        if (version->downloadUrl.isEmpty()) {
            throw ApiError::unsupported(tr("%1 does not allow third-party downloads of this file; download it from the website").arg(pack->name));
        }

        auto* model = ResourceApi::modelFor(instance, kind);
        const bool indexed = !APPLICATION->settings()->get("ModMetadataDisabled").toBool();
        // TODO(webui): resolve and offer required dependencies (GetModDependenciesTask) like ResourceDownloadDialog does.
        Task::Ptr task = makeShared<ResourceDownloadTask>(pack, *version, model, indexed);
        connect(task.get(), &Task::succeeded, model, [model] { model->update(); });
        return QJsonObject{ { "taskId", m_tasks->start(task, "mod.install", tr("Installing %1").arg(pack->name), instance->id()) } };
    });
}

}  // namespace api
