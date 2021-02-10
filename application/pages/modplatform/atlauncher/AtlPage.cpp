#include "AtlPage.h"
#include "ui_AtlPage.h"

#include "dialogs/NewInstanceDialog.h"
#include <modplatform/atlauncher/ATLPackInstallTask.h>
#include <BuildConfig.h>

AtlPage::AtlPage(NewInstanceDialog* dialog, QWidget *parent)
        : QWidget(parent), ui(new Ui::AtlPage), dialog(dialog)
{
    ui->setupUi(this);

    filterModel = new Atl::FilterModel(this);
    listModel = new Atl::ListModel(this);
    filterModel->setSourceModel(listModel);
    ui->packView->setModel(filterModel);
    ui->packView->setSortingEnabled(true);

    ui->packView->header()->hide();
    ui->packView->setIndentation(0);

    for(int i = 0; i < filterModel->getAvailableSortings().size(); i++)
    {
        ui->sortByBox->addItem(filterModel->getAvailableSortings().keys().at(i));
    }
    ui->sortByBox->setCurrentText(filterModel->translateCurrentSorting());

    connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &AtlPage::onSortingSelectionChanged);
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &AtlPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &AtlPage::onVersionSelectionChanged);
}

AtlPage::~AtlPage()
{
    delete ui;
}

bool AtlPage::shouldDisplay() const
{
    return true;
}

void AtlPage::openedImpl()
{
    listModel->request();
}

void AtlPage::suggestCurrent()
{
    if(isOpened) {
        dialog->setSuggestedPack(selected.name, new ATLauncher::PackInstallTask(selected.safeName, selectedVersion));
    }

    auto editedLogoName = selected.safeName;
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/images/%1.png").arg(selected.safeName.toLower());
    listModel->getLogo(selected.safeName, url, [this, editedLogoName](QString logo)
    {
        dialog->setSuggestedIconFromFile(logo, editedLogoName);
    });
}

void AtlPage::onSortingSelectionChanged(QString data)
{
    auto toSet = filterModel->getAvailableSortings().value(data);
    filterModel->setSorting(toSet);
}

void AtlPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        return;
    }

    selected = filterModel->data(first, Qt::UserRole).value<ATLauncher::IndexedPack>();

    ui->packDescription->setHtml(selected.description.replace("\n", "<br>"));

    for(const auto& version : selected.versions) {
        ui->versionSelectionBox->addItem(version.version);
    }

    suggestCurrent();
}

void AtlPage::onVersionSelectionChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = "";
        return;
    }

    selectedVersion = data;
    suggestCurrent();
}
