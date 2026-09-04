// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QAbstractListModel>

#include "ui/pages/modplatform/ResourceModel.h"

class Version;
class MinecraftInstance;

namespace ResourceDownload {

class ShaderPackResourceModel : public ResourceModel {
    Q_OBJECT

   public:
    ShaderPackResourceModel(const MinecraftInstance&, const ResourceAPI*, const QString& debugName, QString metaEntryBase);

    /* Ask the API for more information */
    void searchWithTerm(const QString& term, unsigned int sort);

    [[nodiscard]] QString debugName() const override { return m_debugName; }
    [[nodiscard]] QString metaEntryBase() const override { return m_metaEntryBase; }

   public slots:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(const QModelIndex&) override;
    ResourceAPI::ProjectInfoArgs createInfoArguments(const QModelIndex&) override;

   protected:
    const MinecraftInstance& m_baseInstance;

   private:
    QString m_debugName;
    QString m_metaEntryBase;
};

}  // namespace ResourceDownload
