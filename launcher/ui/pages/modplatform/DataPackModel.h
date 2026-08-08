// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
// SPDX-FileCopyrightText: 2023 TheKodeToad <TheKodeToad@proton.me>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QAbstractListModel>

#include "BaseInstance.h"

#include "ui/pages/modplatform/ResourceModel.h"

class Version;

namespace ResourceDownload {

class DataPackResourceModel : public ResourceModel {
    Q_OBJECT

   public:
    DataPackResourceModel(const BaseInstance&, const ResourceAPI*, const QString&, QString);

    /* Ask the API for more information */
    void searchWithTerm(const QString& term, unsigned int sort);

    [[nodiscard]] QString debugName() const override { return m_debugName; }
    [[nodiscard]] QString metaEntryBase() const override { return m_metaEntryBase; }

   public slots:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(const QModelIndex& /*unused*/) override;
    QString createInfoArguments(const QModelIndex& /*unused*/) override;

   protected:
    const BaseInstance& m_baseInstance;

   private:
    QString m_debugName;
    QString m_metaEntryBase;
};

}  // namespace ResourceDownload
