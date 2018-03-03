#include "ChooseFtbPackDialog.h"
#include <QPushButton>

ChooseFtbPackDialog::ChooseFtbPackDialog(FtbModpackList modpacks) : ui(new Ui::ChooseFtbPackDialog)
{
	ui->setupUi(this);

	filterModel = new FtbFilterModel(this);
	listModel = new FtbListModel(this);
	filterModel->setSourceModel(listModel);
	listModel->fill(modpacks);

	ui->packList->setModel(filterModel);
	ui->packList->setSortingEnabled(true);
	ui->packList->header()->hide();
	ui->packList->setIndentation(0);

	filterModel->setSorting(FtbFilterModel::Sorting::ByName);

	for(int i = 0; i < filterModel->getAvailableSortings().size(); i++){
		ui->sortByBox->addItem(filterModel->getAvailableSortings().keys().at(i));
	}

	ui->sortByBox->setCurrentText(filterModel->getAvailableSortings().key(filterModel->getCurrentSorting()));

	connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &ChooseFtbPackDialog::onSortingSelectionChanged);
	connect(ui->packVersionSelection, &QComboBox::currentTextChanged, this, &ChooseFtbPackDialog::onVersionSelectionItemChanged);
	connect(ui->packList->selectionModel(), &QItemSelectionModel::currentChanged, this, &ChooseFtbPackDialog::onPackSelectionChanged);

	ui->modpackInfo->setOpenExternalLinks(true);

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

ChooseFtbPackDialog::~ChooseFtbPackDialog()
{
	delete ui;
}

void ChooseFtbPackDialog::onPackSelectionChanged(QModelIndex now, QModelIndex prev)
{
	ui->packVersionSelection->clear();
	FtbModpack selectedPack = filterModel->data(now, Qt::UserRole).value<FtbModpack>();

	ui->modpackInfo->setHtml("Pack by <b>" + selectedPack.author + "</b>" + "<br>Minecraft " + selectedPack.mcVersion + "<br>"
				"<br>" + selectedPack.description + "<ul><li>" + selectedPack.mods.replace(";", "</li><li>") + "</li></ul>");

	bool currentAdded = false;

	for(int i = 0; i < selectedPack.oldVersions.size(); i++) {
		if(selectedPack.currentVersion == selectedPack.oldVersions.at(i)) {
			currentAdded = true;
		}
		ui->packVersionSelection->addItem(selectedPack.oldVersions.at(i));
	}

	if(!currentAdded) {
		ui->packVersionSelection->addItem(selectedPack.currentVersion);
	}

	selected = selectedPack;
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!selected.broken);
}

void ChooseFtbPackDialog::onVersionSelectionItemChanged(QString data)
{
	if(data.isNull() || data.isEmpty()) {
		selectedVersion = "";
		return;
	}

	selectedVersion = data;
}

FtbModpack ChooseFtbPackDialog::getSelectedModpack()
{
	return selected;
}

QString ChooseFtbPackDialog::getSelectedVersion()
{
	return selectedVersion;
}

void ChooseFtbPackDialog::onSortingSelectionChanged(QString data)
{
	filterModel->setSorting(filterModel->getAvailableSortings().value(data));
}
