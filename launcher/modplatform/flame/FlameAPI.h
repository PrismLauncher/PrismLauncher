#pragma once

#include "modplatform/ModIndex.h"
#include "modplatform/helpers/NetworkModAPI.h"

class FlameAPI : public NetworkModAPI {
   public:
    auto matchFingerprints(const QList<uint>& fingerprints, QByteArray* response) -> NetJob::Ptr;
    auto getModFileChangelog(int modId, int fileId) -> QString;
    auto getModDescription(int modId) -> QString;

    auto getLatestVersion(VersionSearchArgs&& args) -> ModPlatform::IndexedVersion;

    auto getProjects(QStringList addonIds, QByteArray* response) const -> NetJob* override;
    auto getFiles(QStringList fileIds, QByteArray* response) const -> NetJob*;

   private:
    inline auto getSortFieldInt(QString sortString) const -> int
    {
        return sortString == "Featured"         ? 1
               : sortString == "Popularity"     ? 2
               : sortString == "LastUpdated"    ? 3
               : sortString == "Name"           ? 4
               : sortString == "Author"         ? 5
               : sortString == "TotalDownloads" ? 6
               : sortString == "Category"       ? 7
               : sortString == "GameVersion"    ? 8
                                                : 1;
    }

   private:
    inline auto getModSearchURL(SearchArgs& args) const -> QString override
    {
        auto gameVersionStr = args.versions.size() != 0 ? QString("gameVersion=%1").arg(args.versions.front().toString()) : QString();

        return QString(
                   "https://api.curseforge.com/v1/mods/search?"
                   "gameId=432&"
                   "classId=6&"

                   "index=%1&"
                   "pageSize=25&"
                   "searchFilter=%2&"
                   "sortField=%3&"
                   "sortOrder=desc&"
                   "modLoaderType=%4&"
                   "%5")
            .arg(args.offset)
            .arg(args.search)
            .arg(getSortFieldInt(args.sorting))
            .arg(getMappedModLoader(args.loaders))
            .arg(gameVersionStr);
    };

    inline auto getModInfoURL(QString& id) const -> QString override
    {
        return QString("https://api.curseforge.com/v1/mods/%1").arg(id);
    };

    inline auto getVersionsURL(VersionSearchArgs& args) const -> QString override
    {
        QString gameVersionQuery = args.mcVersions.size() == 1 ? QString("gameVersion=%1&").arg(args.mcVersions.front().toString()) : "";
        QString modLoaderQuery = QString("modLoaderType=%1&").arg(getMappedModLoader(args.loaders));

        return QString("https://api.curseforge.com/v1/mods/%1/files?pageSize=10000&%2%3")
            .arg(args.addonId)
            .arg(gameVersionQuery)
            .arg(modLoaderQuery);
    };

   public:
    static auto getMappedModLoader(const ModLoaderTypes loaders) -> int
    {
        // https://docs.curseforge.com/?http#tocS_ModLoaderType
        if (loaders & Forge)
            return 1;
        if (loaders & Fabric)
            return 4;
        // TODO: remove this once Quilt drops official Fabric support
        if (loaders & Quilt)  // NOTE: Most if not all Fabric mods should work *currently*
            return 4;  // Quilt would probably be 5
        return 0;
    }
};
