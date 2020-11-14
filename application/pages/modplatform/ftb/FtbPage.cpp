#include "FtbPage.h"
#include "ui_FtbPage.h"

#include <QKeyEvent>

#include "dialogs/NewInstanceDialog.h"
#include "modplatform/modpacksch/FTBPackInstallTask.h"

FtbPage::FtbPage(NewInstanceDialog* dialog, QWidget *parent)
        : QWidget(parent), ui(new Ui::FtbPage), dialog(dialog)
{
    ui->setupUi(this);

    filterModel = new Ftb::FilterModel(this);
    listModel = new Ftb::ListModel(this);
    filterModel->setSourceModel(listModel);
    ui->packView->setModel(filterModel);
    ui->packView->setSortingEnabled(true);
    ui->packView->header()->hide();
    ui->packView->setIndentation(0);

    ui->searchEdit->installEventFilter(this);

    for(int i = 0; i < filterModel->getAvailableSortings().size(); i++)
    {
        ui->sortByBox->addItem(filterModel->getAvailableSortings().keys().at(i));
    }
    ui->sortByBox->setCurrentText(filterModel->translateCurrentSorting());

    connect(ui->searchButton, &QPushButton::clicked, this, &FtbPage::triggerSearch);
    connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &FtbPage::onSortingSelectionChanged);
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FtbPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FtbPage::onVersionSelectionChanged);
}

FtbPage::~FtbPage()
{
    delete ui;
}

bool FtbPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool FtbPage::shouldDisplay() const
{
    return true;
}

void FtbPage::openedImpl()
{
    dialog->setSuggestedPack();
    triggerSearch();
}

void FtbPage::suggestCurrent()
{
    if(isOpened)
    {
        dialog->setSuggestedPack(selected.name, new ModpacksCH::PackInstallTask(selected, selectedVersion));

        for(auto art : selected.art) {
            if(art.type == "square") {
                QString editedLogoName;
                editedLogoName = selected.name;

                listModel->getLogo(selected.name, art.url, [this, editedLogoName](QString logo)
                {
                    dialog->setSuggestedIconFromFile(logo + ".small", editedLogoName);
                });
            }
        }
    }
}

void FtbPage::triggerSearch()
{
    listModel->searchWithTerm(ui->searchEdit->text());
}

void FtbPage::onSortingSelectionChanged(QString data)
{
    auto toSet = filterModel->getAvailableSortings().value(data);
    filterModel->setSorting(toSet);
}

void FtbPage::onSelectionChanged(QModelIndex first, QModelIndex second)
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

    selected = filterModel->data(first, Qt::UserRole).value<ModpacksCH::Modpack>();

    // reverse foreach, so that the newest versions are first
    for (auto i = selected.versions.size(); i--;) {
        ui->versionSelectionBox->addItem(selected.versions.at(i).name);
    }

    suggestCurrent();
}

void FtbPage::onVersionSelectionChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = "";
        return;
    }

    selectedVersion = data;
    suggestCurrent();
}
