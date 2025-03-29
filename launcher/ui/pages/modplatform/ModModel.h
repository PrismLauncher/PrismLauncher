// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QAbstractListModel>

#include "BaseInstance.h"

#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"

#include "ui/pages/modplatform/ResourceModel.h"
#include "ui/widgets/ModFilterWidget.h"

class Version;

namespace ResourceDownload {

class ModPage;

class ModModel : public ResourceModel {
    Q_OBJECT

   public:
    ModModel(BaseInstance&, ResourceAPI* api, QString debugName, QString metaEntryBase);

    /* Ask the API for more information */
    void searchWithTerm(const QString& term, unsigned int sort, bool filter_changed);

    void setFilter(std::shared_ptr<ModFilterWidget::Filter> filter) { m_filter = filter; }
    virtual QVariant getInstalledPackVersion(ModPlatform::IndexedPack::Ptr) const override;

    [[nodiscard]] QString debugName() const override { return m_debugName; }
    [[nodiscard]] QString metaEntryBase() const override { return m_metaEntryBase; }

   public slots:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(const QModelIndex&) override;
    ResourceAPI::ProjectInfoArgs createInfoArguments(const QModelIndex&) override;

   protected:
    virtual bool isPackInstalled(ModPlatform::IndexedPack::Ptr) const override;

    virtual bool checkFilters(ModPlatform::IndexedPack::Ptr) override;
    virtual bool checkVersionFilters(const ModPlatform::IndexedVersion&) override;

   protected:
    BaseInstance& m_base_instance;

    std::shared_ptr<ModFilterWidget::Filter> m_filter = nullptr;

   private:
    QString m_debugName;
    QString m_metaEntryBase;
};

}  // namespace ResourceDownload
