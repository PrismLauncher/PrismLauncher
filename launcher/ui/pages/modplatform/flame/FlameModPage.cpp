#include "FlameModPage.h"
#include "ui_ModPage.h"

#include "FlameModModel.h"
#include "ui/dialogs/ModDownloadDialog.h"

FlameModPage::FlameModPage(ModDownloadDialog* dialog, BaseInstance* instance) 
    : ModPage(dialog, instance, new FlameAPI())
{
    listModel = new FlameMod::ListModel(this);
    ui->packView->setModel(listModel);

    // index is used to set the sorting with the flame api
    ui->sortByBox->addItem(tr("Sort by Featured"));
    ui->sortByBox->addItem(tr("Sort by Popularity"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by Name"));
    ui->sortByBox->addItem(tr("Sort by Author"));
    ui->sortByBox->addItem(tr("Sort by Downloads"));

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems, 
    // so it's best not to connect them in the parent's contructor...
    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlameModPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlameModPage::onVersionSelectionChanged);
    connect(ui->modSelectionButton, &QPushButton::clicked, this, &FlameModPage::onModSelected);
}

auto FlameModPage::validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, QString loaderVer) const -> bool
{
    (void) loaderVer;
    return ver.mcVersion.contains(mineVer);
}

// I don't know why, but doing this on the parent class makes it so that
// other mod providers start loading before being selected, at least with
// my Qt, so we need to implement this in every derived class...
auto FlameModPage::shouldDisplay() const -> bool { return true; }
