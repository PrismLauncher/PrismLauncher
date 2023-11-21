// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include "PluginPage.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui_PluginPage.h"

#include "Application.h"
#include "DesktopServices.h"
#include "settings/SettingsObject.h"

PluginPage::PluginPage(QWidget* parent) : QMainWindow(parent), ui(new Ui::PluginPage)
{
    ui->setupUi(this);

    ui->actionsToolbar->insertSpacer(ui->actionViewConfigs);

    m_model = APPLICATION->plugins();

    m_filterModel = m_model->createFilterProxyModel(this);
    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSourceModel(m_model.get());
    m_filterModel->setFilterKeyColumn(-1);
    ui->treeView->setModel(m_filterModel);

    assert(ui->treeView->header()->count() > 0);

    ui->treeView->setResizeModes({ QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Stretch,
                                   QHeaderView::Interactive, QHeaderView::Interactive });

    ui->treeView->installEventFilter(this);
    ui->treeView->sortByColumn(1, Qt::AscendingOrder);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    // connect(ui->actionAddItem, &QAction::triggered, this, &PluginPage::addItem);
    // connect(ui->actionRemoveItem, &QAction::triggered, this, &PluginPage::removeItem);
    connect(ui->actionEnableItem, &QAction::triggered, this, &PluginPage::enableItem);
    connect(ui->actionDisableItem, &QAction::triggered, this, &PluginPage::disableItem);
    // connect(ui->actionViewConfigs, &QAction::triggered, this, &PluginPage::viewConfigs);
    connect(ui->actionViewFolder, &QAction::triggered, this, &PluginPage::viewFolder);

    ui->actionAddItem->setVisible(false);
    ui->actionRemoveItem->setVisible(false);
    ui->actionViewConfigs->setVisible(false);

    connect(ui->treeView, &ModListView::customContextMenuRequested, this, &PluginPage::ShowContextMenu);
    connect(ui->treeView, &ModListView::activated, this, &PluginPage::itemActivated);

    auto selection_model = ui->treeView->selectionModel();
    connect(selection_model, &QItemSelectionModel::currentChanged, this, &PluginPage::current);

    connect(ui->filterEdit, &QLineEdit::textChanged, this, &PluginPage::filterTextChanged);

    auto viewHeader = ui->treeView->header();
    viewHeader->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(viewHeader, &QHeaderView::customContextMenuRequested, this, &PluginPage::ShowHeaderContextMenu);

    loadColumns();
    connect(viewHeader, &QHeaderView::sectionResized, this, [this] { saveColumns(); });
}

PluginPage::~PluginPage()
{
    delete ui;
}

void PluginPage::saveColumns()
{
}

void PluginPage::loadColumns()
{
}

void PluginPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->actionsToolbar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->treeView->mapToGlobal(pos));
    delete menu;
}

void PluginPage::ShowHeaderContextMenu(const QPoint& pos)
{
    auto menu = new QMenu(ui->treeView);

    menu->addSeparator()->setText(tr("Show / Hide Columns"));

    {
        auto act = new QAction(menu);
        act->setText(tr("Image"));
        act->setCheckable(true);
        act->setChecked(!ui->treeView->isColumnHidden(1));
        connect(act, &QAction::toggled, ui->treeView, [this](bool toggled) {
            ui->treeView->setColumnHidden(1, !toggled);
            saveColumns();
        });
        menu->addAction(act);
    }

    menu->exec(ui->treeView->mapToGlobal(pos));
    menu->deleteLater();
}

void PluginPage::itemActivated(const QModelIndex&)
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    if (m_model->setEnabled(selection.indexes(), Plugin::EnableAction::TOGGLE)) {
        auto response = CustomMessageBox::selectable(this, "Need Launcher restart",
                            "Some settings / disable of plugins only gets effective on the next restart.",
                            QMessageBox::Warning, QMessageBox::Ok, QMessageBox::Ok)
            ->exec();
    }
}

void PluginPage::filterTextChanged(const QString& newContents)
{
    m_viewFilter = newContents;
    m_filterModel->setFilterRegularExpression(m_viewFilter);
}

void PluginPage::enableItem()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    m_model->setEnabled(selection.indexes(), Plugin::EnableAction::ENABLE);
}

void PluginPage::disableItem()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    if (m_model->setEnabled(selection.indexes(), Plugin::EnableAction::DISABLE)) {
        auto response = CustomMessageBox::selectable(this, "Need Launcher restart",
                            "Some settings / disable of plugins only gets effective on the next restart.",
                            QMessageBox::Warning, QMessageBox::Ok, QMessageBox::Ok)
            ->exec();
    }
}

void PluginPage::viewFolder()
{
    DesktopServices::openDirectory(m_model->dir().absolutePath(), true);
}

bool PluginPage::current(const QModelIndex& current, const QModelIndex& previous)
{
    if (!current.isValid()) {
        ui->frame->clear();
        return false;
    }

    return onSelectionChanged(current, previous);
}

bool PluginPage::onSelectionChanged(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    auto sourceCurrent = m_filterModel->mapToSource(current);
    int row = sourceCurrent.row();
    Plugin const& plugin = *m_model->at(row);

    QString name = plugin.name().isEmpty() ? plugin.id() : plugin.name();
    QString link = plugin.metaurl();
    QString text = link.isEmpty() ? name : ("<a href=\"" + link + "\">" + name + "</a>");
    if (!plugin.authors().isEmpty())
        text += " by " + plugin.authors().join(", ");
    ui->frame->setName(text);

    ui->frame->setDescription(plugin.description());

    ui->frame->setImage(plugin.icon({ 64, 64 }));

    if (!plugin.license().isEmpty()) {
        // TODO: maybe support multiple licenses via a object like ModLicense
        ui->frame->setLicense(tr("License: %1").arg(plugin.license()));
    }

    if (!plugin.issueTracker().isEmpty()) {
        ui->frame->setIssueTracker(tr("Report issues to: ")
            + "<a href=\"" + plugin.issueTracker() + "\">" + plugin.issueTracker() + "</a>");
    }

    return true;
}
