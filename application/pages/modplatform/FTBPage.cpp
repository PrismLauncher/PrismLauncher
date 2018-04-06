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

void FTBPage::onPublicPackSelectionChanged(QModelIndex first, QModelIndex second)
{
	onPackSelectionChanged(first, second, publicFilterModel);
}

void FTBPage::onThirdPartyPackSelectionChanged(QModelIndex first, QModelIndex second)
{
	onPackSelectionChanged(first, second, thirdPartyFilterModel);
}

void FTBPage::onPackSelectionChanged(QModelIndex now, QModelIndex prev, FtbFilterModel *model)
{
	ui->packVersionSelection->clear();
	FtbModpack selectedPack = model->data(now, Qt::UserRole).value<FtbModpack>();

	ui->modpackInfo->setHtml("Pack by <b>" + selectedPack.author + "</b>" + "<br>Minecraft " + selectedPack.mcVersion + "<br>"
				"<br>" + selectedPack.description + "<ul><li>" + selectedPack.mods.replace(";", "</li><li>") + "</li></ul>");

	bool currentAdded = false;

	for(int i = 0; i < selectedPack.oldVersions.size(); i++)
	{
		if(selectedPack.currentVersion == selectedPack.oldVersions.at(i))
		{
			currentAdded = true;
		}
		ui->packVersionSelection->addItem(selectedPack.oldVersions.at(i));
	}

	if(!currentAdded)
	{
		ui->packVersionSelection->addItem(selectedPack.currentVersion);
	}

	selected = selectedPack;
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

FtbModpack FTBPage::getSelectedModpack()
{
	return selected;
}

QString FTBPage::getSelectedVersion()
{
	return selectedVersion;
}

void FTBPage::onSortingSelectionChanged(QString data)
{
	FtbFilterModel::Sorting toSet = publicFilterModel->getAvailableSortings().value(data);
	publicFilterModel->setSorting(toSet);
	thirdPartyFilterModel->setSorting(toSet);
}

void FTBPage::onTabChanged(int tab)
{
	ui->publicPackList->selectionModel()->reset();
	ui->thirdPartyPackList->selectionModel()->reset();
}
