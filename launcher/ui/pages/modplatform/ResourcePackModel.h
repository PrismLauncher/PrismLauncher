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
    ResourcePackResourceModel(BaseInstance const&, ResourceAPI*, QString debugName, QString metaEntryBase);

    /* Ask the API for more information */
    void searchWithTerm(const QString& term, unsigned int sort);

    [[nodiscard]] QString debugName() const override { return m_debugName; }
    [[nodiscard]] QString metaEntryBase() const override { return m_metaEntryBase; }

   public slots:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(const QModelIndex&) override;
    ResourceAPI::ProjectInfoArgs createInfoArguments(const QModelIndex&) override;

   protected:
    const BaseInstance& m_base_instance;

   private:
    QString m_debugName;
    QString m_metaEntryBase;
};

}  // namespace ResourceDownload
