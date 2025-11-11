// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
// SPDX-FileCopyrightText: 2023 TheKodeToad <TheKodeToad@proton.me>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "DataPackPage.h"
#include "ui_ResourcePage.h"

#include "DataPackModel.h"

#include "ui/dialogs/ResourceDownloadDialog.h"

#include <QRegularExpression>

namespace ResourceDownload {

DataPackResourcePage::DataPackResourcePage(DataPackDownloadDialog* dialog, BaseInstance& instance) : ResourcePage(dialog, instance) {}

/******** Callbacks to events in the UI (set up in the derived classes) ********/

void DataPackResourcePage::triggerSearch()
{
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();

    updateSelectionButton();

    static_cast<DataPackResourceModel*>(m_model)->searchWithTerm(getSearchTerm(), m_ui->sortByBox->currentData().toUInt());
    m_fetchProgress.watch(m_model->activeSearchJob().get());
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
