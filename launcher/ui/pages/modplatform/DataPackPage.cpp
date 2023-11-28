// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
// SPDX-FileCopyrightText: 2023 TheKodeToad <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "DataPackPage.h"
#include "modplatform/ModIndex.h"
#include "ui_ResourcePage.h"

#include "DataPackModel.h"

#include "ui/dialogs/ResourceDownloadDialog.h"

#include <QRegularExpression>

namespace ResourceDownload {

DataPackResourcePage::DataPackResourcePage(DataPackDownloadDialog* dialog, BaseInstance& instance) : ResourcePage(dialog, instance)
{
    connect(m_ui->searchButton, &QPushButton::clicked, this, &DataPackResourcePage::triggerSearch);
    connect(m_ui->packView, &QListView::doubleClicked, this, &DataPackResourcePage::onResourceSelected);
}

/******** Callbacks to events in the UI (set up in the derived classes) ********/

void DataPackResourcePage::triggerSearch()
{
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();

    updateSelectionButton();

    static_cast<DataPackResourceModel*>(m_model)->searchWithTerm(getSearchTerm(), m_ui->sortByBox->currentData().toUInt());
    m_fetch_progress.watch(m_model->activeSearchJob().get());
}

QMap<QString, QString> DataPackResourcePage::urlHandlers() const
{
    QMap<QString, QString> map;
    map.insert(QRegularExpression::anchoredPattern("(?:www\\.)?modrinth\\.com\\/resourcepack\\/([^\\/]+)\\/?"), "modrinth");
    map.insert(QRegularExpression::anchoredPattern("(?:www\\.)?curseforge\\.com\\/minecraft\\/texture-packs\\/([^\\/]+)\\/?"),
               "curseforge");
    map.insert(QRegularExpression::anchoredPattern("minecraft\\.curseforge\\.com\\/projects\\/([^\\/]+)\\/?"), "curseforge");
    return map;
}

}  // namespace ResourceDownload
