/* Copyright 2013-2018 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "NewModFolderPage.h"
#include "ui_NewModFolderPage.h"

#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QAbstractItemModel>

#include "MultiMC.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/ModEditDialogCommon.h"
#include <GuiUtil.h>
#include "minecraft/ModsModel.h"
#include "minecraft/Mod.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/ComponentList.h"
#include <DesktopServices.h>

NewModFolderPage::NewModFolderPage(BaseInstance *inst, std::shared_ptr<ModsModel> mods, QString id,
                             QString iconName, QString displayName, QString helpPage,
                             QWidget *parent)
    : QWidget(parent), ui(new Ui::NewModFolderPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    m_inst = inst;
    m_mods = mods;
    m_id = id;
    m_displayName = displayName;
    m_iconName = iconName;
    m_helpName = helpPage;
    m_fileSelectionFilter = "%1 (*.zip *.jar)";
    m_filterModel = new QSortFilterProxyModel(this);
    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSourceModel(m_mods.get());
    m_filterModel->setFilterKeyColumn(-1);
    ui->modTreeView->setModel(m_filterModel);
    ui->modTreeView->installEventFilter(this);
    ui->modTreeView->sortByColumn(1, Qt::AscendingOrder);
    auto smodel = ui->modTreeView->selectionModel();
    connect(smodel, &QItemSelectionModel::currentChanged, this, &NewModFolderPage::modCurrent);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &NewModFolderPage::on_filterTextChanged );
}

void NewModFolderPage::openedImpl()
{
    m_mods->startWatching();
}

void NewModFolderPage::closedImpl()
{
    m_mods->stopWatching();
}

void NewModFolderPage::on_filterTextChanged(const QString& newContents)
{
    m_viewFilter = newContents;
    m_filterModel->setFilterFixedString(m_viewFilter);
}


NewModFolderPage::~NewModFolderPage()
{
    m_mods->stopWatching();
    delete ui;
}

bool NewModFolderPage::shouldDisplay() const
{
    if (m_inst)
        return !m_inst->isRunning();
    return true;
}

bool NewModFolderPage::modListFilter(QKeyEvent *keyEvent)
{
    switch (keyEvent->key())
    {
    case Qt::Key_Delete:
        on_rmModBtn_clicked();
        return true;
    case Qt::Key_Plus:
        on_addModBtn_clicked();
        return true;
    default:
        break;
    }
    return QWidget::eventFilter(ui->modTreeView, keyEvent);
}

bool NewModFolderPage::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() != QEvent::KeyPress)
    {
        return QWidget::eventFilter(obj, ev);
    }
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
    if (obj == ui->modTreeView)
        return modListFilter(keyEvent);
    return QWidget::eventFilter(obj, ev);
}

void NewModFolderPage::on_addModBtn_clicked()
{
    auto list = GuiUtil::BrowseForFiles(
        m_helpName,
        tr("Select %1",
           "Select whatever type of files the page contains. Example: 'Loader Mods'")
            .arg(m_displayName),
        m_fileSelectionFilter.arg(m_displayName), MMC->settings()->get("CentralModsDir").toString(),
        this->parentWidget());
    if (!list.empty())
    {
        for (auto filename : list)
        {
            m_mods->installMod(filename);
        }
    }
}

void NewModFolderPage::on_enableModBtn_clicked()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->enableMods(selection.indexes(), true);
}

void NewModFolderPage::on_disableModBtn_clicked()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->enableMods(selection.indexes(), false);
}

void NewModFolderPage::on_rmModBtn_clicked()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->deleteMods(selection.indexes());
}

void NewModFolderPage::on_configFolderBtn_clicked()
{
    DesktopServices::openDirectory(m_inst->instanceConfigFolder(), true);
}

void NewModFolderPage::on_viewModBtn_clicked()
{
    DesktopServices::openDirectory(m_mods->dir().absolutePath(), true);
}

void NewModFolderPage::modCurrent(const QModelIndex &current, const QModelIndex &previous)
{
    if (!current.isValid())
    {
        ui->frame->clear();
        return;
    }
    auto sourceCurrent = m_filterModel->mapToSource(current);
    int row = sourceCurrent.row();
    Mod &m = m_mods->operator[](row);
    ui->frame->updateWithMod(m);
}
