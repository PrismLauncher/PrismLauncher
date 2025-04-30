// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <memory>
#include "api/structures/ModLoader.h"
#include "api/structures/Provider.h"
#include "modplatform/ResourceAPI.h"
#include "net/NetJob.h"

class FlameAPI : public ResourceAPI {
   public:
    std::optional<Platform::Version> getLatestVersion(QList<Platform::Version> versions,
                                                      QList<Platform::ModLoader> instanceLoaders,
                                                      Platform::ModLoaders fallback);

    NetJob::Ptr getFiles(const QStringList& fileIds, std::shared_ptr<QByteArray> response) const;
    Task::Ptr getFile(const QString& addonId, const QString& fileId, std::shared_ptr<QByteArray> response) const;

    Platform::Provider provider() const override { return Platform::Provider::FLAME; }
};
