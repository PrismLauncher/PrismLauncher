#pragma once
#include <QDateTime>
#include <QList>
#include <QString>

#include <QDebug>

#include "Version.h"

struct GitHubReleaseAsset {
    int id = -1;
    QString name;
    QString label;
    QString content_type;
    int size;
    QDateTime created_at;
    QDateTime updated_at;
    QString browser_download_url;

    bool isValid() { return id > 0; }
};

struct GitHubRelease {
    int id = -1;
    QString name;
    QString tag_name;
    QDateTime created_at;
    QDateTime published_at;
    bool prerelease;
    bool draft;
    QString body;
    QList<GitHubReleaseAsset> assets;
    Version version;

    bool isValid() const { return id > 0; }
};

QDebug operator<<(QDebug debug, const GitHubReleaseAsset& rls);
QDebug operator<<(QDebug debug, const GitHubRelease& rls);

