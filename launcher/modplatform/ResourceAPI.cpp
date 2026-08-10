#include "modplatform/ResourceAPI.h"

#include <algorithm>

#include "Json.h"

#include "modplatform/ModIndex.h"

#include "net/RPCSink.h"

Net::Spec<QList<ModPlatform::IndexedPack::Ptr>> ResourceAPI::searchProjects(const SearchArgs& args) const
{
    auto searchUrlOptional = getSearchURL(args);
    if (!searchUrlOptional.has_value()) {
        return {};
    }

    const auto& searchUrl = searchUrlOptional.value();
    auto parseFunc = [this](const QByteArray& response) -> Net::RpcSink<QList<ModPlatform::IndexedPack::Ptr>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from" << debugName() << "at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;

            return std::unexpected(parseError.errorString());
        }

        QList<ModPlatform::IndexedPack::Ptr> newList;
        auto packs = documentToArray(doc);

        for (auto packRaw : packs) {
            auto packObj = packRaw.toObject();

            ModPlatform::IndexedPack::Ptr pack = std::make_shared<ModPlatform::IndexedPack>();
            try {
                loadIndexedPack(*pack, packObj);
                newList << pack;
            } catch (const JSONValidationError& e) {
                qWarning().nospace() << "Error while loading resource from " << debugName() << ": " << e.cause();
                continue;
            }
        }
        return newList;
    };

    return Net::Spec<QList<ModPlatform::IndexedPack::Ptr>>{ .url = QUrl(searchUrl),
                                                            .parse = parseFunc,
                                                            .name = "ResourceAPI::searchProjects" };
}

Net::Spec<QVector<ModPlatform::IndexedVersion>> ResourceAPI::getProjectVersions(const VersionSearchArgs& args) const
{
    auto versionsUrlOptional = getVersionsURL(args);
    if (!versionsUrlOptional.has_value()) {
        return {};
    }

    auto parseFunc = [this, args](const QByteArray& response) -> Net::RpcSink<QVector<ModPlatform::IndexedVersion>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for getting versions at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }

        QVector<ModPlatform::IndexedVersion> unsortedVersions;
        try {
            auto arr = doc.isObject() ? doc.object()["data"].toArray() : doc.array();

            for (auto versionIter : arr) {
                auto obj = versionIter.toObject();

                auto file = loadIndexedPackVersion(obj, args.resourceType);
                if (!file.addonId.isValid()) {
                    file.addonId = args.pack->addonId;
                }

                if (file.fileId.isValid() && !file.downloadUrl.isEmpty()) {  // Heuristic to check if the returned value is valid
                    unsortedVersions.append(file);
                }
            }

            auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
                // dates are in RFC 3339 format
                return a.date > b.date;
            };
            std::ranges::sort(unsortedVersions, orderSortPredicate);
        } catch (const JSONValidationError& e) {
            qDebug() << doc;
            qWarning() << "Error while reading" << debugName() << "resource version:" << e.cause();
            return std::unexpected(e.cause());
        }

        return unsortedVersions;
    };

    return Net::Spec<QVector<ModPlatform::IndexedVersion>>{ .url = QUrl(versionsUrlOptional.value()),
                                                            .parse = parseFunc,
                                                            .name = "ResourceAPI::getProjectVersions" };
}

Net::Spec<ModPlatform::IndexedVersion> ResourceAPI::getDependencyVersion(const DependencySearchArgs& args) const
{
    auto urlOptional = getDependencyURL(args);
    if (!urlOptional.has_value()) {
        return {};
    }

    auto parseFunc = [this, args](const QByteArray& response) -> Net::RpcSink<ModPlatform::IndexedVersion>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for getting dependency version at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }

        QJsonArray arr;
        if (args.dependency.version.length() != 0 && doc.isObject()) {
            arr.append(doc.object());
        } else {
            arr = doc.isObject() ? doc.object()["data"].toArray() : doc.array();
        }

        QVector<ModPlatform::IndexedVersion> versions;
        for (auto versionIter : arr) {
            auto obj = versionIter.toObject();

            auto file = loadIndexedPackVersion(obj, ModPlatform::ResourceType::Mod);
            if (!file.addonId.isValid()) {
                file.addonId = args.dependency.addonId;
            }

            if (file.fileId.isValid() &&
                (!file.loaders || args.loader & file.loaders)) {  // Heuristic to check if the returned value is valid
                versions.append(file);
            }
        }

        auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
            // dates are in RFC 3339 format
            return a.date > b.date;
        };
        std::ranges::sort(versions, orderSortPredicate);
        auto bestMatch = versions.size() != 0 ? versions.front() : ModPlatform::IndexedVersion();
        return bestMatch;
    };

    return Net::Spec<ModPlatform::IndexedVersion>{ .url = QUrl(urlOptional.value()),
                                                   .parse = parseFunc,
                                                   .name = "ResourceAPI::getDependencyVersion" };
}

QString ResourceAPI::getGameVersionsString(const std::vector<Version>& mcVersions)
{
    QString s;
    for (const auto& ver : mcVersions) {
        s += QString("\"%1\",").arg(mapMCVersionToModrinth(ver));
    }
    s.remove(s.length() - 1, 1);  // remove last comma
    return s;
}

QString ResourceAPI::mapMCVersionToModrinth(const Version& v)
{
    static const QString s_preString = " Pre-Release ";
    auto verStr = v.toString();

    if (verStr.contains(s_preString)) {
        verStr.replace(s_preString, "-pre");
    }
    verStr.replace(" ", "-");
    return verStr;
}

Net::Spec<ModPlatform::IndexedPack::Ptr> ResourceAPI::getProject(const QString& addonId, bool includeExtra) const
{
    auto projectUrlOptional = getInfoURL(addonId);
    if (!projectUrlOptional.has_value()) {
        return {};
    }

    const auto& projectUrl = projectUrlOptional.value();

    auto parseFunc = [this, includeExtra](const QByteArray& response) -> Net::RpcSink<ModPlatform::IndexedPack::Ptr>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for project info at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }
        try {
            auto obj = Json::requireObject(doc);
            if (obj.contains("data")) {
                obj = Json::requireObject(obj, "data");
            }
            auto pack = std::make_shared<ModPlatform::IndexedPack>();
            loadIndexedPack(*pack, obj);
            if (includeExtra) {
                loadExtraPackInfo(*pack, obj);
            }
            return pack;
        } catch (const JSONValidationError& e) {
            qDebug() << doc;
            qWarning() << "Error while reading" << debugName() << "resource info:" << e.cause();
            return std::unexpected(e.cause());
        }
    };

    return Net::Spec<ModPlatform::IndexedPack::Ptr>{ .url = QUrl(projectUrl), .parse = parseFunc, .name = "ResourceAPI::getProject" };
}