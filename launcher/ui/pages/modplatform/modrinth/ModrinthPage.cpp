#include "ModrinthPage.h"
#include "ui_ModPage.h"

#include "ModrinthModel.h"
#include "ui/dialogs/ModDownloadDialog.h"

ModrinthPage::ModrinthPage(ModDownloadDialog* dialog, BaseInstance* instance)
    : ModPage(dialog, instance, new ModrinthAPI())
{
    listModel = new Modrinth::ListModel(this);
    ui->packView->setModel(listModel);

    // index is used to set the sorting with the modrinth api
    ui->sortByBox->addItem(tr("Sort by Relevence"));
    ui->sortByBox->addItem(tr("Sort by Downloads"));
    ui->sortByBox->addItem(tr("Sort by Follows"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by newest"));

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems, 
    // so it's best not to connect them in the parent's contructor...
    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthPage::onVersionSelectionChanged);
    connect(ui->modSelectionButton, &QPushButton::clicked, this, &ModrinthPage::onModSelected);
}

auto ModrinthPage::validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, QString loaderVer) const -> bool
{
    return ver.mcVersion.contains(mineVer) && ver.loaders.contains(loaderVer);
}

// I don't know why, but doing this on the parent class makes it so that
// other mod providers start loading before being selected, at least with
// my Qt, so we need to implement this in every derived class...
auto ModrinthPage::shouldDisplay() const -> bool { return true; }
