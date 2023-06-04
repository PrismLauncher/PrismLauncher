#include "GitHubRelease.h"

QDebug operator<<(QDebug debug, const GitHubReleaseAsset& asset)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "GitHubReleaseAsset( " 
        "id: " << asset.id << ", "
        "name " << asset.name << ", "
        "label: " << asset.label << ", "
        "content_type: " << asset.content_type << ", "
        "size: " << asset.size << ", "
        "created_at: " << asset.created_at << ", "
        "updated_at: " << asset.updated_at << ", "
        "browser_download_url: " << asset.browser_download_url << " "
        ")";
    return debug;
}

QDebug operator<<(QDebug debug, const GitHubRelease& rls)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "GitHubRelease( " 
        "id: " << rls.id << ", "
        "name " << rls.name << ", "
        "tag_name: " << rls.tag_name << ", "
        "created_at: " << rls.created_at << ", "
        "published_at: " << rls.published_at << ", "
        "prerelease: " << rls.prerelease << ", "
        "draft: " << rls.draft << ", "
        "version" << rls.version << ", "
        "body: " << rls.body << ", "
        "assets: " << rls.assets << " "
        ")";
    return debug;
}

