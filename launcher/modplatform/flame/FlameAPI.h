// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <memory>
#include "api/structures/Category.h"
#include "api/structures/ModLoader.h"
#include "api/structures/Provider.h"
#include "modplatform/ResourceAPI.h"
#include "net/NetJob.h"

class FlameAPI : public ResourceAPI {
   public:
    QString getModFileChangelog(int modId, int fileId);

    std::optional<Platform::Version> getLatestVersion(QList<Platform::Version> versions,
                                                      QList<Platform::ModLoader> instanceLoaders,
                                                      Platform::ModLoaders fallback);

    Task::Ptr matchFingerprints(const QList<uint>& fingerprints, std::shared_ptr<QByteArray> response);
    NetJob::Ptr getFiles(const QStringList& fileIds, std::shared_ptr<QByteArray> response) const;
    Task::Ptr getFile(const QString& addonId, const QString& fileId, std::shared_ptr<QByteArray> response) const;

    static Task::Ptr getCategories(std::shared_ptr<QByteArray> response, Platform::ResourceType type);
    static Task::Ptr getModCategories(std::shared_ptr<QByteArray> response);
    static QList<Platform::Category> loadModCategories(std::shared_ptr<QByteArray> response);

    Platform::Provider provider() const override { return Platform::Provider::FLAME; }

   private:
    static int getClassId(Platform::ResourceType type)
    {
        switch (type) {
            default:
            case Platform::ResourceType::Mod:
                return 6;
            case Platform::ResourceType::ResourcePack:
                return 12;
            case Platform::ResourceType::ShaderPack:
                return 6552;
            case Platform::ResourceType::Modpack:
                return 4471;
        }
    }
};
