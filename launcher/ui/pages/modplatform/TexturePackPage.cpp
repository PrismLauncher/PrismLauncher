// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "TexturePackPage.h"
#include "ui_ResourcePage.h"

#include "TexturePackModel.h"

#include "ui/dialogs/ResourceDownloadDialog.h"

#include <QRegularExpression>
#include <utility>

namespace {

ResourceDownload::ResourceDescriptor prepareResourcePackDescriptor()
{
    QMap<QString, QString> urlHandlers;
    urlHandlers.insert(QRegularExpression::anchoredPattern(R"((?:www\.)?modrinth\.com\/resourcepack\/([^\/]+)\/?)"), "modrinth");
    urlHandlers.insert(QRegularExpression::anchoredPattern(R"((?:www\.)?curseforge\.com\/minecraft\/texture-packs\/([^\/]+)\/?)"),
                       "curseforge");
    urlHandlers.insert(QRegularExpression::anchoredPattern(R"(minecraft\.curseforge\.com\/projects\/([^\/]+)\/?)"), "curseforge");
    return {
        .helpPage = {},
        //: The singular version of 'texture packs'
        .resourceString = QObject::tr("texture pack"),
        //: The plural version of 'texture pack'
        .resourcesString = QObject::tr("texture packs"),
        .supportsFiltering = false,
        .isIndexed = true,
        .urlHandlers = urlHandlers,
    };
}
}  // namespace
namespace ResourceDownload {

TexturePackResourcePage::TexturePackResourcePage(ResourceDownloadDialog* dialog,
                                                 BaseInstance& instance,
                                                 ResourceProviderData provider,
                                                 const ResourceAPI* api,
                                                 TexturePackResourceModel* model)
    : ResourcePage(dialog, instance, prepareResourcePackDescriptor(), std::move(provider))
{
    m_model = model;
    if (!m_model) {
        m_model = new TexturePackResourceModel(instance, api, debugName(), metaEntryBase());
    }
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's constructor...
    connect(m_model, &ResourceModel::versionListUpdated, this, &ResourcePage::versionListUpdated);
    connect(m_model, &ResourceModel::projectInfoUpdated, this, &ResourcePage::updateUi);
    connect(m_model, &QAbstractListModel::modelReset, this, &ResourcePage::modelReset);

    connect(m_ui->sortByBox, &QComboBox::currentIndexChanged, this, &TexturePackResourcePage::triggerSearch);
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TexturePackResourcePage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentIndexChanged, this, &TexturePackResourcePage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &TexturePackResourcePage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

/******** Callbacks to events in the UI (set up in the derived classes) ********/

void TexturePackResourcePage::triggerSearch()
{
    m_ui->packView->selectionModel()->setCurrentIndex({}, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();

    updateSelectionButton();

    static_cast<ResourcePackResourceModel*>(m_model)->searchWithTerm(getSearchTerm(), m_ui->sortByBox->currentData().toUInt());
    m_fetchProgress.watch(m_model->activeSearchJob().get());
}

}  // namespace ResourceDownload
