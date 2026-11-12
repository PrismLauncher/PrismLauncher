// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Prism Launcher Contributors
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

#include "GithubAPI.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>
#include <utility>

#include "Application.h"
#include "BuildConfig.h"
#include "Json.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

namespace {

/** A filesystem-safe slug for a "owner/repo" id, used as the metadata file name. */
QString slugFromId(const QString& id)
{
    auto slug = id;
    slug.replace('/', "__");
    return slug;
}

/** Best-effort extraction of Minecraft versions from a release tag / title. */
QStringList parseMcVersions(const QString& text)
{
    static const QRegularExpression versionRegex{ QStringLiteral(R"(\d+\.\d+(?:\.\d+)?)") };
    QStringList found;
    auto iter = versionRegex.globalMatch(text);
    while (iter.hasNext()) {
        const auto match = iter.next().captured(0);
        if (!found.contains(match)) {
            found << match;
        }
    }
    return found;
}

Result<ModPlatform::IndexedPack> loadIndexedPack(const QJsonObject& obj)
{
    ModPlatform::IndexedPack pack;

    TRY_INTO(const auto fullName, Json::requireString(obj, "full_name"))
    TRY_INTO(const auto htmlUrl, Json::requireString(obj, "html_url"))
    TRY_INTO(const auto name, Json::requireString(obj, "name"))

    pack.provider = ModPlatform::ResourceProvider::GITHUB;
    pack.addonId = fullName;
    pack.slug = slugFromId(fullName);
    pack.name = name;
    pack.description = obj.value("description").toString();
    pack.websiteUrl = htmlUrl;
    pack.logoUrl = obj.value("owner").toObject().value("avatar_url").toString();

    const auto owner = obj.value("owner").toObject().value("login").toString();
    if (!owner.isEmpty()) {
        pack.authors = { { .name = owner, .url = QStringLiteral("https://github.com/%1").arg(owner) } };
    }

    pack.extraDataLoaded = false;
    return pack;
}

}  // namespace

QList<ResourceAPI::SortingMethod> GithubAPI::getSortingMethods() const
{
    // https://docs.github.com/en/rest/search/search#search-repositories
    // Index 0 (no sorting entry) means "best match", the GitHub default.
    return { { .index = 1, .name = "stars", .readableName = QObject::tr("Sort by Stars") },
             { .index = 2, .name = "updated", .readableName = QObject::tr("Sort by Last Updated") } };
}

std::pair<Task::Ptr, QByteArray*> GithubAPI::getProjects(QStringList) const
{
    // The generic download flow never batches project requests for GitHub, and the GitHub
    // API has no batch endpoint. Return an empty, immediately-succeeding job.
    auto netJob = makeShared<NetJob>(QStringLiteral("Github::GetProjects"), APPLICATION->network());
    m_emptyResponse = "[]";
    return { netJob, &m_emptyResponse };
}

std::pair<Task::Ptr, QByteArray*> GithubAPI::getModCategories() const
{
    auto netJob = makeShared<NetJob>(QStringLiteral("Github::GetCategories"), APPLICATION->network());
    m_emptyResponse = "[]";
    return { netJob, &m_emptyResponse };
}

QList<ModPlatform::Category> GithubAPI::loadModCategories(const QByteArray&) const
{
    // GitHub has no mod category system.
    return {};
}

QList<Net::HeaderPair> GithubAPI::headers()
{
    static const QList<Net::HeaderPair> headers{
        { "User-Agent", BuildConfig.USER_AGENT.toUtf8() },
        { "Accept", "application/vnd.github+json" },
    };
    return headers;
}

Net::RPC::Spec<ModPlatform::IndexedPack> GithubAPI::getProject(const QString& id) const
{
    // https://docs.github.com/en/rest/repos/repos#get-a-repository
    return { { .url = Github::apiUrl().toString() + QStringLiteral("/repos/") + id, .headers = GithubAPI::headers() },
             [](const auto& response) -> Result<ModPlatform::IndexedPack> {
                 TRY_INTO(const auto obj, Json::requireObject(response, "GitHub repository"))
                 TRY_INTO(auto pack, loadIndexedPack(obj))

                 pack.extraDataLoaded = true;
                 pack.extraData.sourceUrl = obj.value("html_url").toString();
                 pack.extraData.issuesUrl = obj.value("issues_url").toString().replace(QStringLiteral("{/number}"), QString());
                 pack.extraData.wikiUrl = obj.value("has_wiki").toBool() ? obj.value("html_url").toString() + QStringLiteral("/wiki")
                                                                         : QString();
                 return pack;
             } };
}

Net::RPC::Spec<QList<ModPlatform::IndexedPack>> GithubAPI::searchProjects(const SearchArgs& args) const
{
    // https://docs.github.com/en/rest/search/search#search-repositories
    QUrl url(Github::apiUrl().toString() + QStringLiteral("/search/repositories"));
    QUrlQuery query;

    QString searchTerm = args.search.value_or(QString()).trimmed();
    if (searchTerm.isEmpty()) {
        searchTerm = QStringLiteral("minecraft");
    }
    query.addQueryItem(QStringLiteral("q"), searchTerm + QStringLiteral(" minecraft in:name,description,readme"));

    const auto page = args.offset / Github::searchPageSize() + 1;
    query.addQueryItem(QStringLiteral("per_page"), QString::number(Github::searchPageSize()));
    query.addQueryItem(QStringLiteral("page"), QString::number(page));

    if (args.sorting.has_value()) {
        query.addQueryItem(QStringLiteral("sort"), args.sorting->name);
        query.addQueryItem(QStringLiteral("order"), QStringLiteral("desc"));
    }
    url.setQuery(query);

    return { { .url = url, .headers = GithubAPI::headers() }, [](const auto& response) -> Result<QList<ModPlatform::IndexedPack>> {
                QList<ModPlatform::IndexedPack> newList;
                TRY_INTO(const auto doc, Json::requireDocument(response, "GitHub repository search")
                                             .and_then([](const auto& v) { return Json::requireObject(v); })
                                             .and_then([](const auto& v) { return Json::requireArray(v, "items"); }))

                for (const auto& packRaw : doc) {
                    auto packObj = packRaw.toObject();
                    TRY_INTO(auto pack, loadIndexedPack(packObj))
                    newList << pack;
                }
                return newList;
            } };
}

auto GithubAPI::getVersionsURL(const VersionSearchArgs& args) const -> std::optional<QString>
{
    // https://docs.github.com/en/rest/releases/releases#list-releases
    return Github::apiUrl().toString() + QStringLiteral("/repos/") + args.pack->addonId.toString() + QStringLiteral("/releases?per_page=30");
}

std::optional<QString> GithubAPI::getDependencyURL(const DependencySearchArgs&) const
{
    // GitHub releases carry no dependency information.
    return std::nullopt;
}

Result<ModPlatform::IndexedVersion> GithubAPI::loadIndexedPackVersion(QJsonObject& obj, ModPlatform::ResourceType) const
{
    // https://docs.github.com/en/rest/releases/releases#get-a-release
    const auto assets = obj.value("assets").toArray();

    QJsonObject chosenAsset;
    for (const auto& assetRaw : assets) {
        const auto asset = assetRaw.toObject();
        const auto assetName = asset.value("name").toString();
        if (assetName.endsWith(QStringLiteral(".jar"), Qt::CaseInsensitive) && !assetName.contains(QStringLiteral("-sources")) &&
            !assetName.contains(QStringLiteral("-javadoc"))) {
            chosenAsset = asset;
            break;
        }
    }

    if (chosenAsset.isEmpty()) {
        return std::unexpected{ QObject::tr("Release '%1' has no downloadable mod file (.jar asset)")
                                    .arg(obj.value("tag_name").toString()) };
    }

    ModPlatform::IndexedVersion version;

    const auto tag = obj.value("tag_name").toString();
    version.versionType = obj.value("prerelease").toBool() ? ModPlatform::IndexedVersionType::Beta
                                                           : ModPlatform::IndexedVersionType::Release;
    version.versionNumber = tag;

    auto releaseName = obj.value("name").toString();
    version.version = releaseName.isEmpty() ? tag : releaseName;

    version.addonId = QVariant::fromValue(obj.value("id").toVariant().toLongLong());
    version.fileId = QVariant::fromValue(chosenAsset.value("id").toVariant().toLongLong());
    version.downloadUrl = chosenAsset.value("browser_download_url").toString();
    version.date = obj.value("published_at").toString();
    version.fileName = chosenAsset.value("name").toString();
    version.changelog = obj.value("body").toString();
    // GitHub does not tag assets with Minecraft versions; parse what we can from the release.
    version.mcVersion = parseMcVersions(tag + QLatin1Char(' ') + releaseName);

    return version;
}
