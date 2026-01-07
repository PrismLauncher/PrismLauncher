// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderPackPage.h"
#include "modplatform/ModIndex.h"
#include "ui_ResourcePage.h"

#include "ShaderPackModel.h"

#include "Application.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

#include <QRegularExpression>

namespace ResourceDownload {

ShaderPackResourcePage::ShaderPackResourcePage(ShaderPackDownloadDialog* dialog, BaseInstance& instance) : ResourcePage(dialog, instance) {}

/******** Callbacks to events in the UI (set up in the derived classes) ********/

void ShaderPackResourcePage::triggerSearch()
{
    m_ui->packView->selectionModel()->setCurrentIndex({}, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();

    updateSelectionButton();

    static_cast<ShaderPackResourceModel*>(m_model)->searchWithTerm(getSearchTerm(), m_ui->sortByBox->currentData().toUInt());
    m_fetchProgress.watch(m_model->activeSearchJob().get());
}

QMap<QString, QString> ShaderPackResourcePage::urlHandlers() const
{
    QMap<QString, QString> map;
    map.insert(QRegularExpression::anchoredPattern("(?:www\\.)?modrinth\\.com\\/shaders\\/([^\\/]+)\\/?"), "modrinth");
    map.insert(QRegularExpression::anchoredPattern("(?:www\\.)?curseforge\\.com\\/minecraft\\/customization\\/([^\\/]+)\\/?"),
               "curseforge");
    map.insert(QRegularExpression::anchoredPattern("minecraft\\.curseforge\\.com\\/projects\\/([^\\/]+)\\/?"), "curseforge");
    return map;
}

void ShaderPackResourcePage::addResourceToPage(ModPlatform::IndexedPack::Ptr pack,
                                               ModPlatform::IndexedVersion& version,
                                               ResourceFolderModel* base_model)
{
    bool is_indexed = !APPLICATION->settings()->get("ModMetadataDisabled").toBool();
    m_model->addPack(pack, version, base_model, is_indexed);
}

}  // namespace ResourceDownload
