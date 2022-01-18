/* Copyright 2013-2021 MultiMC Contributors
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

#include "ModFolderPage.h"
#include "ui_ModFolderPage.h"

#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QAbstractItemModel>
#include <QMenu>
#include <QSortFilterProxyModel>

#include "Application.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ModDownloadDialog.h"
#include "ui/GuiUtil.h"

#include "DesktopServices.h"

#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/PackProfile.h"

#include "Version.h"
#include "ui/dialogs/ProgressDialog.h"

namespace {
    // FIXME: wasteful
    void RemoveThePrefix(QString & string) {
        QRegularExpression regex(QStringLiteral("^(([Tt][Hh][eE])|([Tt][eE][Hh])) +"));
        string.remove(regex);
        string = string.trimmed();
    }
}

class ModSortProxy : public QSortFilterProxyModel
{
public:
    explicit ModSortProxy(QObject *parent = 0) : QSortFilterProxyModel(parent)
    {
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override {
        ModFolderModel *model = qobject_cast<ModFolderModel *>(sourceModel());
        if(!model) {
            return false;
        }
        const auto &mod = model->at(source_row);
        if(mod.name().contains(filterRegExp())) {
            return true;
        }
        if(mod.description().contains(filterRegExp())) {
            return true;
        }
        for(auto & author: mod.authors()) {
            if (author.contains(filterRegExp())) {
                return true;
            }
        }
        return false;
    }

    bool lessThan(const QModelIndex & source_left, const QModelIndex & source_right) const override
    {
        ModFolderModel *model = qobject_cast<ModFolderModel *>(sourceModel());
        if(
            !model ||
            !source_left.isValid() ||
            !source_right.isValid() ||
            source_left.column() != source_right.column()
        ) {
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }

        // we are now guaranteed to have two valid indexes in the same column... we love the provided invariants unconditionally and proceed.

        auto column = (ModFolderModel::Columns) source_left.column();
        bool invert = false;
        switch(column) {
            // GH-2550 - sort by enabled/disabled
            case ModFolderModel::ActiveColumn: {
                auto dataL = source_left.data(Qt::CheckStateRole).toBool();
                auto dataR = source_right.data(Qt::CheckStateRole).toBool();
                if(dataL != dataR) {
                    return dataL > dataR;
                }
                // fallthrough
                invert = sortOrder() == Qt::DescendingOrder;
            }
            // GH-2722 - sort mod names in a way that discards "The" prefixes
            case ModFolderModel::NameColumn: {
                auto dataL = model->data(model->index(source_left.row(), ModFolderModel::NameColumn)).toString();
                RemoveThePrefix(dataL);
                auto dataR = model->data(model->index(source_right.row(), ModFolderModel::NameColumn)).toString();
                RemoveThePrefix(dataR);

                auto less = dataL.compare(dataR, sortCaseSensitivity());
                if(less != 0) {
                    return invert ? (less > 0) : (less < 0);
                }
                // fallthrough
                invert = sortOrder() == Qt::DescendingOrder;
            }
            // GH-2762 - sort versions by parsing them as versions
            case ModFolderModel::VersionColumn: {
                auto dataL = Version(model->data(model->index(source_left.row(), ModFolderModel::VersionColumn)).toString());
                auto dataR = Version(model->data(model->index(source_right.row(), ModFolderModel::VersionColumn)).toString());
                return invert ? (dataL > dataR) : (dataL < dataR);
            }
            default: {
                return QSortFilterProxyModel::lessThan(source_left, source_right);
            }
        }
    }
};

ModFolderPage::ModFolderPage(
    BaseInstance *inst,
    std::shared_ptr<ModFolderModel> mods,
    QString id,
    QString iconName,
    QString displayName,
    QString helpPage,
    QWidget *parent
) :
    QMainWindow(parent),
    ui(new Ui::ModFolderPage)
{
    ui->setupUi(this);
    ui->actionsToolbar->insertSpacer(ui->actionView_configs);

    m_inst = inst;
    on_RunningState_changed(m_inst && m_inst->isRunning());
    m_mods = mods;
    m_id = id;
    m_displayName = displayName;
    m_iconName = iconName;
    m_helpName = helpPage;
    m_fileSelectionFilter = "%1 (*.zip *.jar)";
    m_filterModel = new ModSortProxy(this);
    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSourceModel(m_mods.get());
    m_filterModel->setFilterKeyColumn(-1);
    ui->modTreeView->setModel(m_filterModel);
    ui->modTreeView->installEventFilter(this);
    ui->modTreeView->sortByColumn(1, Qt::AscendingOrder);
    ui->modTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->modTreeView, &ModListView::customContextMenuRequested, this, &ModFolderPage::ShowContextMenu);
    connect(ui->modTreeView, &ModListView::activated, this, &ModFolderPage::modItemActivated);

    auto smodel = ui->modTreeView->selectionModel();
    connect(smodel, &QItemSelectionModel::currentChanged, this, &ModFolderPage::modCurrent);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &ModFolderPage::on_filterTextChanged);
    connect(m_inst, &BaseInstance::runningStatusChanged, this, &ModFolderPage::on_RunningState_changed);
}

void ModFolderPage::modItemActivated(const QModelIndex&)
{
    if(!m_controlsEnabled) {
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->setModStatus(selection.indexes(), ModFolderModel::Toggle);
}

QMenu * ModFolderPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->actionsToolbar->toggleViewAction() );
    return filteredMenu;
}

void ModFolderPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->actionsToolbar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->modTreeView->mapToGlobal(pos));
    delete menu;
}

void ModFolderPage::openedImpl()
{
    m_mods->startWatching();
}

void ModFolderPage::closedImpl()
{
    m_mods->stopWatching();
}

void ModFolderPage::on_filterTextChanged(const QString& newContents)
{
    m_viewFilter = newContents;
    m_filterModel->setFilterFixedString(m_viewFilter);
}


CoreModFolderPage::CoreModFolderPage(BaseInstance *inst, std::shared_ptr<ModFolderModel> mods,
                                     QString id, QString iconName, QString displayName,
                                     QString helpPage, QWidget *parent)
    : ModFolderPage(inst, mods, id, iconName, displayName, helpPage, parent)
{
}

ModFolderPage::~ModFolderPage()
{
    m_mods->stopWatching();
    delete ui;
}

void ModFolderPage::on_RunningState_changed(bool running)
{
    if(m_controlsEnabled == !running) {
        return;
    }
    m_controlsEnabled = !running;
    ui->actionAdd->setEnabled(m_controlsEnabled);
    ui->actionDisable->setEnabled(m_controlsEnabled);
    ui->actionEnable->setEnabled(m_controlsEnabled);
    ui->actionRemove->setEnabled(m_controlsEnabled);
}

bool ModFolderPage::shouldDisplay() const
{
    return true;
}

bool CoreModFolderPage::shouldDisplay() const
{
    if (ModFolderPage::shouldDisplay())
    {
        auto inst = dynamic_cast<MinecraftInstance *>(m_inst);
        if (!inst)
            return true;
        auto version = inst->getPackProfile();
        if (!version)
            return true;
        if(!version->getComponent("net.minecraftforge"))
        {
            return false;
        }
        if(!version->getComponent("net.minecraft"))
        {
            return false;
        }
        if(version->getComponent("net.minecraft")->getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate)
        {
            return true;
        }
    }
    return false;
}

bool ModFolderPage::modListFilter(QKeyEvent *keyEvent)
{
    switch (keyEvent->key())
    {
    case Qt::Key_Delete:
        on_actionRemove_triggered();
        return true;
    case Qt::Key_Plus:
        on_actionAdd_triggered();
        return true;
    default:
        break;
    }
    return QWidget::eventFilter(ui->modTreeView, keyEvent);
}

bool ModFolderPage::eventFilter(QObject *obj, QEvent *ev)
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

void ModFolderPage::on_actionAdd_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    auto list = GuiUtil::BrowseForFiles(
        m_helpName,
        tr("Select %1",
           "Select whatever type of files the page contains. Example: 'Loader Mods'")
            .arg(m_displayName),
        m_fileSelectionFilter.arg(m_displayName), APPLICATION->settings()->get("CentralModsDir").toString(),
        this->parentWidget());
    if (!list.empty())
    {
        for (auto filename : list)
        {
            m_mods->installMod(filename);
        }
    }
}

void ModFolderPage::on_actionEnable_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->setModStatus(selection.indexes(), ModFolderModel::Enable);
}

void ModFolderPage::on_actionDisable_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->setModStatus(selection.indexes(), ModFolderModel::Disable);
}

void ModFolderPage::on_actionRemove_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->modTreeView->selectionModel()->selection());
    m_mods->deleteMods(selection.indexes());
}

void ModFolderPage::on_actionInstall_mods_triggered()
{
    if(!m_controlsEnabled) {
        return;
    }
    if(m_inst->typeName() != "Minecraft"){
        return; //this is a null instance or a legacy instance
    }
    bool hasFabric = !((MinecraftInstance *)m_inst)->getPackProfile()->getComponentVersion("net.fabricmc.fabric-loader").isEmpty();
    bool hasForge = !((MinecraftInstance *)m_inst)->getPackProfile()->getComponentVersion("net.minecraftforge").isEmpty();
    if (!hasFabric && !hasForge) {
        QMessageBox::critical(this,tr("Error"),tr("Please install a mod loader first !"));
        return;
    }
    ModDownloadDialog mdownload(m_mods, this, m_inst);
    if(mdownload.exec()) {
        ModDownloadTask *task = mdownload.getTask();
        if (task) {
            connect(task, &Task::failed, [this, task](QString reason) {
                task->deleteLater();
                CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
            });
            connect(task, &Task::succeeded, [this, task]() {
                QStringList warnings = task->warnings();
                if (warnings.count()) {
                    CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'),
                                                 QMessageBox::Warning)->show();
                }
                task->deleteLater();
            });
            ProgressDialog loadDialog(this);
            loadDialog.setSkipButton(true, tr("Abort"));
            loadDialog.execWithTask(task);
            m_mods->update();
        }
    }
}

void ModFolderPage::on_actionView_configs_triggered()
{
    DesktopServices::openDirectory(m_inst->instanceConfigFolder(), true);
}

void ModFolderPage::on_actionView_Folder_triggered()
{
    DesktopServices::openDirectory(m_mods->dir().absolutePath(), true);
}

void ModFolderPage::modCurrent(const QModelIndex &current, const QModelIndex &previous)
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
