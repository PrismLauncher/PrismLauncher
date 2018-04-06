#include "FTBPage.h"
#include "ui_FTBPage.h"

#include "MultiMC.h"
#include "FolderInstanceProvider.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/NewInstanceDialog.h"
#include "modplatform/ftb/FtbPackFetchTask.h"
#include "modplatform/ftb/FtbPackInstallTask.h"
#include "FtbListModel.h"

FTBPage::FTBPage(NewInstanceDialog* dialog, QWidget *parent)
	: QWidget(parent), dialog(dialog), ui(new Ui::FTBPage)
{
	ftbFetchTask = new FtbPackFetchTask();

	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

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

	connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &FTBPage::onSortingSelectionChanged);
	connect(ui->packVersionSelection, &QComboBox::currentTextChanged, this, &FTBPage::onVersionSelectionItemChanged);

	connect(ui->publicPackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &FTBPage::onPublicPackSelectionChanged);
	connect(ui->thirdPartyPackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &FTBPage::onThirdPartyPackSelectionChanged);

	connect(ui->ftbTabWidget, &QTabWidget::currentChanged, this, &FTBPage::onTabChanged);

	ui->modpackInfo->setOpenExternalLinks(true);

	ui->publicPackList->selectionModel()->reset();
	ui->thirdPartyPackList->selectionModel()->reset();
}

FTBPage::~FTBPage()
{
	delete ui;
	if(ftbFetchTask) {
		ftbFetchTask->deleteLater();
	}
}

bool FTBPage::shouldDisplay() const
{
	return true;
}

void FTBPage::openedImpl()
{
	if(!initialized)
	{
		connect(ftbFetchTask, &FtbPackFetchTask::finished, this, &FTBPage::ftbPackDataDownloadSuccessfully);
		connect(ftbFetchTask, &FtbPackFetchTask::failed, this, &FTBPage::ftbPackDataDownloadFailed);
		ftbFetchTask->fetch();
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

void FTBPage::onPackSelectionChanged(FtbModpack* pack)
{
	ui->packVersionSelection->clear();
	if(pack)
	{
		ui->modpackInfo->setHtml("Pack by <b>" + pack->author + "</b>" + "<br>Minecraft " + pack->mcVersion + "<br>"
			"<br>" + pack->description + "<ul><li>" + pack->mods.replace(";", "</li><li>") + "</li></ul>");
		bool currentAdded = false;

		for(int i = 0; i < pack->oldVersions.size(); i++)
		{
			if(pack->currentVersion == pack->oldVersions.at(i))
			{
				currentAdded = true;
			}
			ui->packVersionSelection->addItem(pack->oldVersions.at(i));
		}

		if(!currentAdded)
		{
			ui->packVersionSelection->addItem(pack->currentVersion);
		}
		selected = *pack;
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
}

void FTBPage::onSortingSelectionChanged(QString data)
{
	FtbFilterModel::Sorting toSet = publicFilterModel->getAvailableSortings().value(data);
	publicFilterModel->setSorting(toSet);
	thirdPartyFilterModel->setSorting(toSet);
}

void FTBPage::onTabChanged(int tab)
{
	FtbFilterModel* currentModel = nullptr;
	QTreeView* currentList = nullptr;
	if (tab == 0)
	{
		currentModel = publicFilterModel;
		currentList = ui->publicPackList;
	}
	else
	{
		currentModel = thirdPartyFilterModel;
		currentList = ui->thirdPartyPackList;
	}
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
