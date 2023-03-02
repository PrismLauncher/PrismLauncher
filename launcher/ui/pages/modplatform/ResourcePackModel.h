// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QAbstractListModel>

#include "BaseInstance.h"

#include "modplatform/ModIndex.h"

#include "ui/pages/modplatform/ResourceModel.h"

class Version;

namespace ResourceDownload {

class ResourcePackResourceModel : public ResourceModel {
    Q_OBJECT

   public:
    ResourcePackResourceModel(BaseInstance const&, ResourceAPI*);

    /* Ask the API for more information */
    void searchWithTerm(const QString& term, unsigned int sort);

    void loadIndexedPack(ModPlatform::IndexedPack&, QJsonObject&) override = 0;
    void loadExtraPackInfo(ModPlatform::IndexedPack&, QJsonObject&) override = 0;
    void loadIndexedPackVersions(ModPlatform::IndexedPack&, QJsonArray&) override = 0;

   public slots:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) override;
    ResourceAPI::ProjectInfoArgs createInfoArguments(QModelIndex&) override;

   protected:
    const BaseInstance& m_base_instance;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override = 0;
};

}  // namespace ResourceDownload
