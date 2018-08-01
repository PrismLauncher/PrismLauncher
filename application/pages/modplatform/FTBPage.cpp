#include "FTBPage.h"
#include "ui_FTBPage.h"

#include <QInputDialog>

#include "MultiMC.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/NewInstanceDialog.h"
#include "modplatform/ftb/FtbPackFetchTask.h"
#include "modplatform/ftb/FtbPackInstallTask.h"
#include "modplatform/ftb/FtbPrivatePackManager.h"
#include "FtbListModel.h"

FTBPage::FTBPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), dialog(dialog), ui(new Ui::FTBPage)
{
    ftbFetchTask.reset(new FtbPackFetchTask());
    ftbPrivatePacks.reset(new FtbPrivatePackManager());

    ui->setupUi(this);

    {
        publicFilterModel = new FtbFilterModel(this);
        publicListModel = new FtbListModel(this);
        publicFilterModel->setSourceModel(publicListModel);

        ui->publicPackList->setModel(publicFilterModel);
        ui->publicPackList->setSortingEnabled(true);
        ui->publicPackList->header()->hide();
        ui->publicPackList->setIndentation(0);
        ui->publicPackList->setIconSize(QSize(42, 42));

        for(int i = 0; i < publicFilterModel->getAvailableSortings().size(); i++)
        {
            ui->sortByBox->addItem(publicFilterModel->getAvailableSortings().keys().at(i));
        }

        ui->sortByBox->setCurrentText(publicFilterModel->translateCurrentSorting());
    }

    {
        thirdPartyFilterModel = new FtbFilterModel(this);
        thirdPartyModel = new FtbListModel(this);
        thirdPartyFilterModel->setSourceModel(thirdPartyModel);

        ui->thirdPartyPackList->setModel(thirdPartyFilterModel);
        ui->thirdPartyPackList->setSortingEnabled(true);
        ui->thirdPartyPackList->header()->hide();
        ui->thirdPartyPackList->setIndentation(0);
        ui->thirdPartyPackList->setIconSize(QSize(42, 42));

        thirdPartyFilterModel->setSorting(publicFilterModel->getCurrentSorting());
    }

    {
        privateFilterModel = new FtbFilterModel(this);
        privateListModel = new FtbListModel(this);
        privateFilterModel->setSourceModel(privateListModel);

        ui->privatePackList->setModel(privateFilterModel);
        ui->privatePackList->setSortingEnabled(true);
        ui->privatePackList->header()->hide();
        ui->privatePackList->setIndentation(0);
        ui->privatePackList->setIconSize(QSize(42, 42));

        privateFilterModel->setSorting(publicFilterModel->getCurrentSorting());
    }

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &FTBPage::onSortingSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FTBPage::onVersionSelectionItemChanged);

    connect(ui->publicPackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &FTBPage::onPublicPackSelectionChanged);
    connect(ui->thirdPartyPackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &FTBPage::onThirdPartyPackSelectionChanged);
    connect(ui->privatePackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &FTBPage::onPrivatePackSelectionChanged);

    connect(ui->addPackBtn, &QPushButton::pressed, this, &FTBPage::onAddPackClicked);
    connect(ui->removePackBtn, &QPushButton::pressed, this, &FTBPage::onRemovePackClicked);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &FTBPage::onTabChanged);

    // ui->modpackInfo->setOpenExternalLinks(true);

    ui->publicPackList->selectionModel()->reset();
    ui->thirdPartyPackList->selectionModel()->reset();
    ui->privatePackList->selectionModel()->reset();

    onTabChanged(ui->tabWidget->currentIndex());
}

FTBPage::~FTBPage()
{
    delete ui;
}

bool FTBPage::shouldDisplay() const
{
    return true;
}

void FTBPage::openedImpl()
{
    if(!initialized)
    {
        connect(ftbFetchTask.get(), &FtbPackFetchTask::finished, this, &FTBPage::ftbPackDataDownloadSuccessfully);
        connect(ftbFetchTask.get(), &FtbPackFetchTask::failed, this, &FTBPage::ftbPackDataDownloadFailed);

        connect(ftbFetchTask.get(), &FtbPackFetchTask::privateFileDownloadFinished, this, &FTBPage::ftbPrivatePackDataDownloadSuccessfully);
        connect(ftbFetchTask.get(), &FtbPackFetchTask::privateFileDownloadFailed, this, &FTBPage::ftbPrivatePackDataDownloadFailed);

        ftbFetchTask->fetch();
        ftbPrivatePacks->load();
        ftbFetchTask->fetchPrivate(ftbPrivatePacks->getCurrentPackCodes().toList());
        initialized = true;
    }
    suggestCurrent();
}

void FTBPage::suggestCurrent()
{
    if(isOpened)
    {
        if(!selected.broken)
        {
            dialog->setSuggestedPack(selected.name, new FtbPackInstallTask(selected, selectedVersion));
            QString editedLogoName;
            if(selected.logo.toLower().startsWith("ftb"))
            {
                editedLogoName = selected.logo;
            }
            else
            {
                editedLogoName = "ftb_" + selected.logo;
            }

            editedLogoName = editedLogoName.left(editedLogoName.lastIndexOf(".png"));

            if(selected.type == FtbPackType::Public)
            {
                publicListModel->getLogo(selected.logo, [this, editedLogoName](QString logo)
                {
                    dialog->setSuggestedIconFromFile(logo, editedLogoName);
                });
            }
            else if (selected.type == FtbPackType::ThirdParty)
            {
                thirdPartyModel->getLogo(selected.logo, [this, editedLogoName](QString logo)
                {
                    dialog->setSuggestedIconFromFile(logo, editedLogoName);
                });
            }
            else if (selected.type == FtbPackType::Private)
            {
                privateListModel->getLogo(selected.logo, [this, editedLogoName](QString logo)
                {
                    dialog->setSuggestedIconFromFile(logo, editedLogoName);
                });
            }
        }
        else
        {
            dialog->setSuggestedPack();
        }
    }
}

void FTBPage::ftbPackDataDownloadSuccessfully(FtbModpackList publicPacks, FtbModpackList thirdPartyPacks)
{
    publicListModel->fill(publicPacks);
    thirdPartyModel->fill(thirdPartyPacks);
}

void FTBPage::ftbPackDataDownloadFailed(QString reason)
{
    //TODO: Display the error
}

void FTBPage::ftbPrivatePackDataDownloadSuccessfully(FtbModpack pack)
{
    privateListModel->addPack(pack);
}

void FTBPage::ftbPrivatePackDataDownloadFailed(QString reason, QString packCode)
{
    auto reply = QMessageBox::question(
        this,
        tr("FTB private packs"),
        tr("Failed to download pack information for code %1.\nShould it be removed now?").arg(packCode)
    );
    if(reply == QMessageBox::Yes)
    {
        ftbPrivatePacks->remove(packCode);
    }
}

void FTBPage::onPublicPackSelectionChanged(QModelIndex now, QModelIndex prev)
{
    if(!now.isValid())
    {
        onPackSelectionChanged();
        return;
    }
    FtbModpack selectedPack = publicFilterModel->data(now, Qt::UserRole).value<FtbModpack>();
    onPackSelectionChanged(&selectedPack);
}

void FTBPage::onThirdPartyPackSelectionChanged(QModelIndex now, QModelIndex prev)
{
    if(!now.isValid())
    {
        onPackSelectionChanged();
        return;
    }
    FtbModpack selectedPack = thirdPartyFilterModel->data(now, Qt::UserRole).value<FtbModpack>();
    onPackSelectionChanged(&selectedPack);
}

void FTBPage::onPrivatePackSelectionChanged(QModelIndex now, QModelIndex prev)
{
    if(!now.isValid())
    {
        onPackSelectionChanged();
        return;
    }
    FtbModpack selectedPack = privateFilterModel->data(now, Qt::UserRole).value<FtbModpack>();
    onPackSelectionChanged(&selectedPack);
}

void FTBPage::onPackSelectionChanged(FtbModpack* pack)
{
    ui->versionSelectionBox->clear();
    if(pack)
    {
        currentModpackInfo->setHtml("Pack by <b>" + pack->author + "</b>" +
                                    "<br>Minecraft " + pack->mcVersion + "<br>" + "<br>" + pack->description + "<ul><li>" + pack->mods.replace(";", "</li><li>")
                                    + "</li></ul>");
        bool currentAdded = false;

        for(int i = 0; i < pack->oldVersions.size(); i++)
        {
            if(pack->currentVersion == pack->oldVersions.at(i))
            {
                currentAdded = true;
            }
            ui->versionSelectionBox->addItem(pack->oldVersions.at(i));
        }

        if(!currentAdded)
        {
            ui->versionSelectionBox->addItem(pack->currentVersion);
        }
        selected = *pack;
    }
    else
    {
        currentModpackInfo->setHtml("");
        ui->versionSelectionBox->clear();
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        return;
    }
    suggestCurrent();
}

void FTBPage::onVersionSelectionItemChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = "";
        return;
    }

    selectedVersion = data;
    suggestCurrent();
}

void FTBPage::onSortingSelectionChanged(QString data)
{
    FtbFilterModel::Sorting toSet = publicFilterModel->getAvailableSortings().value(data);
    publicFilterModel->setSorting(toSet);
    thirdPartyFilterModel->setSorting(toSet);
    privateFilterModel->setSorting(toSet);
}

void FTBPage::onTabChanged(int tab)
{
    if(tab == 1)
    {
        currentModel = thirdPartyFilterModel;
        currentList = ui->thirdPartyPackList;
        currentModpackInfo = ui->thirdPartyPackDescription;
    }
    else if(tab == 2)
    {
        currentModel = privateFilterModel;
        currentList = ui->privatePackList;
        currentModpackInfo = ui->privatePackDescription;
    }
    else
    {
        currentModel = publicFilterModel;
        currentList = ui->publicPackList;
        currentModpackInfo = ui->publicPackDescription;
    }

    currentList->selectionModel()->reset();
    QModelIndex idx = currentList->currentIndex();
    if(idx.isValid())
    {
        auto pack = currentModel->data(idx, Qt::UserRole).value<FtbModpack>();
        onPackSelectionChanged(&pack);
    }
    else
    {
        onPackSelectionChanged();
    }
}

void FTBPage::onAddPackClicked()
{
    bool ok;
    QString text = QInputDialog::getText(
        this,
        tr("Add FTB pack"),
        tr("Enter pack code:"),
        QLineEdit::Normal,
        QString(),
        &ok
    );
    if(ok && !text.isEmpty())
    {
        ftbPrivatePacks->add(text);
        ftbFetchTask->fetchPrivate({text});
    }
}

void FTBPage::onRemovePackClicked()
{
    auto index = ui->privatePackList->currentIndex();
    if(!index.isValid())
    {
        return;
    }
    auto row = index.row();
    FtbModpack pack = privateListModel->at(row);
    auto answer = QMessageBox::question(
        this,
        tr("Remove pack"),
        tr("Are you sure you want to remove pack %1?").arg(pack.name),
        QMessageBox::Yes | QMessageBox::No
    );
    if(answer != QMessageBox::Yes)
    {
        return;
    }

    ftbPrivatePacks->remove(pack.packCode);
    privateListModel->remove(row);
    onPackSelectionChanged();
}
