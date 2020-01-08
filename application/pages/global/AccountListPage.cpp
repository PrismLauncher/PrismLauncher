/* Copyright 2013-2019 MultiMC Contributors
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

#include "AccountListPage.h"
#include "ui_AccountListPage.h"

#include <QItemSelectionModel>
#include <QMenu>

#include <QDebug>

#include "net/NetJob.h"
#include "net/URLConstants.h"
#include "Env.h"

#include "dialogs/ProgressDialog.h"
#include "dialogs/LoginDialog.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/SkinUploadDialog.h"
#include "tasks/Task.h"
#include "minecraft/auth/YggdrasilTask.h"

#include "MultiMC.h"

AccountListPage::AccountListPage(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AccountListPage)
{
    ui->setupUi(this);
    ui->listView->setEmptyString(tr(
        "Welcome!\n"
        "If you're new here, you can click the \"Add\" button to add your Mojang or Minecraft account."
    ));
    ui->listView->setEmptyMode(VersionListView::String);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_accounts = MMC->accounts();

    ui->listView->setModel(m_accounts.get());
    ui->listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Expand the account column
    ui->listView->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    QItemSelectionModel *selectionModel = ui->listView->selectionModel();

    connect(selectionModel, &QItemSelectionModel::selectionChanged, [this](const QItemSelection &sel, const QItemSelection &dsel) {
        updateButtonStates();
    });
    connect(ui->listView, &VersionListView::customContextMenuRequested, this, &AccountListPage::ShowContextMenu);

    connect(m_accounts.get(), SIGNAL(listChanged()), SLOT(listChanged()));
    connect(m_accounts.get(), SIGNAL(activeAccountChanged()), SLOT(listChanged()));

    updateButtonStates();
}

AccountListPage::~AccountListPage()
{
    delete ui;
}

void AccountListPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->toolBar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->listView->mapToGlobal(pos));
    delete menu;
}

void AccountListPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
    QMainWindow::changeEvent(event);
}

QMenu * AccountListPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->toolBar->toggleViewAction() );
    return filteredMenu;
}


void AccountListPage::listChanged()
{
    updateButtonStates();
}

void AccountListPage::on_actionAdd_triggered()
{
    addAccount(tr("Please enter your Minecraft account email and password to add your account."));
}

void AccountListPage::on_actionRemove_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        m_accounts->removeAccount(selected);
    }
}

void AccountListPage::on_actionSetDefault_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        MojangAccountPtr account =
            selected.data(MojangAccountList::PointerRole).value<MojangAccountPtr>();
        m_accounts->setActiveAccount(account->username());
    }
}

void AccountListPage::on_actionNoDefault_triggered()
{
    m_accounts->setActiveAccount("");
}

void AccountListPage::updateButtonStates()
{
    // If there is no selection, disable buttons that require something selected.
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();

    ui->actionRemove->setEnabled(selection.size() > 0);
    ui->actionSetDefault->setEnabled(selection.size() > 0);
    ui->actionUploadSkin->setEnabled(selection.size() > 0);

    if(m_accounts->activeAccount().get() == nullptr) {
        ui->actionNoDefault->setEnabled(false);
        ui->actionNoDefault->setChecked(true);
    }
    else {
        ui->actionNoDefault->setEnabled(true);
        ui->actionNoDefault->setChecked(false);
    }

}

void AccountListPage::addAccount(const QString &errMsg)
{
    // TODO: The login dialog isn't quite done yet
    MojangAccountPtr account = LoginDialog::newAccount(this, errMsg);

    if (account != nullptr)
    {
        m_accounts->addAccount(account);
        if (m_accounts->count() == 1)
            m_accounts->setActiveAccount(account->username());

        // Grab associated player skins
        auto job = new NetJob("Player skins: " + account->username());

        for (AccountProfile profile : account->profiles())
        {
            auto meta = Env::getInstance().metacache()->resolveEntry("skins", profile.id + ".png");
            auto action = Net::Download::makeCached(QUrl(URLConstants::SKINS_BASE + profile.id + ".png"), meta);
            job->addNetAction(action);
            meta->setStale(true);
        }

        job->start();
    }
}

void AccountListPage::on_actionUploadSkin_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        QModelIndex selected = selection.first();
        MojangAccountPtr account = selected.data(MojangAccountList::PointerRole).value<MojangAccountPtr>();
        SkinUploadDialog dialog(account, this);
        dialog.exec();
    }
}
