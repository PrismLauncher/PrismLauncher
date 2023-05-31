// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderPackPage.h"
#include "ui_ResourcePage.h"

#include "ShaderPackModel.h"

#include "ui/dialogs/ResourceDownloadDialog.h"

#include <QRegularExpression>

namespace ResourceDownload {

ShaderPackResourcePage::ShaderPackResourcePage(ShaderPackDownloadDialog* dialog, BaseInstance& instance)
    : ResourcePage(dialog, instance)
{
    connect(m_ui->searchButton, &QPushButton::clicked, this, &ShaderPackResourcePage::triggerSearch);
    connect(m_ui->packView, &QListView::doubleClicked, this, &ShaderPackResourcePage::onResourceSelected);
}

/******** Callbacks to events in the UI (set up in the derived classes) ********/

void ShaderPackResourcePage::triggerSearch()
{
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();

    updateSelectionButton();

    static_cast<ShaderPackResourceModel*>(m_model)->searchWithTerm(getSearchTerm(), m_ui->sortByBox->currentData().toUInt());
    m_fetch_progress.watch(m_model->activeSearchJob().get());
}

QMap<QString, QString> ShaderPackResourcePage::urlHandlers() const
{
    QMap<QString, QString> map;
    map.insert(QRegularExpression::anchoredPattern("(?:www\\.)?modrinth\\.com\\/shaders\\/([^\\/]+)\\/?"), "modrinth");
    map.insert(QRegularExpression::anchoredPattern("(?:www\\.)?curseforge\\.com\\/minecraft\\/customization\\/([^\\/]+)\\/?"), "curseforge");
    map.insert(QRegularExpression::anchoredPattern("minecraft\\.curseforge\\.com\\/projects\\/([^\\/]+)\\/?"), "curseforge");
    return map;
}

void ShaderPackResourcePage::addResourceToDialog(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& version)
{
    if (version.loaders.contains(QStringLiteral("canvas")))
        version.custom_target_folder = QStringLiteral("resourcepacks");

    m_parent_dialog->addResource(pack, version);
}

}  // namespace ResourceDownload
