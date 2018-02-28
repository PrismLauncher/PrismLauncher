#include "ChooseFtbPackDialog.h"
#include "widgets/FtbModpackListItem.h"

ChooseFtbPackDialog::ChooseFtbPackDialog(FtbModpackList modpacks) : ui(new Ui::ChooseFtbPackDialog) {
	ui->setupUi(this);

	for(int i = 0; i < modpacks.size(); i++) {
		FtbModpackListItem *item = new FtbModpackListItem(ui->packList, modpacks.at(i));

		item->setText(modpacks.at(i).name);
	}

	//TODO: Use a model/view instead of a widget
	connect(ui->packList, &QListWidget::itemClicked, this, &ChooseFtbPackDialog::onListItemClicked);
	connect(ui->packVersionSelection, &QComboBox::currentTextChanged, this, &ChooseFtbPackDialog::onVersionSelectionItemChanged);

	ui->modpackInfo->setOpenExternalLinks(true);

}

ChooseFtbPackDialog::~ChooseFtbPackDialog(){
	delete ui;
}

void ChooseFtbPackDialog::onListItemClicked(QListWidgetItem *item){
	ui->packVersionSelection->clear();
	FtbModpack selectedPack = static_cast<FtbModpackListItem*>(item)->getModpack();

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

}

void ChooseFtbPackDialog::onVersionSelectionItemChanged(QString data) {
	if(data.isNull() || data.isEmpty()) {
		selectedVersion = "";
		return;
	}

	selectedVersion = data;
}

FtbModpack ChooseFtbPackDialog::getSelectedModpack() {
	return selected;
}

QString ChooseFtbPackDialog::getSelectedVersion() {
	return selectedVersion;
}
