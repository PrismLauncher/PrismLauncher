#include "FtbPage.h"
#include "ui_FtbPage.h"

#include <QKeyEvent>

#include "dialogs/NewInstanceDialog.h"
#include "modplatform/modpacksch/PackInstallTask.h"

FtbPage::FtbPage(NewInstanceDialog* dialog, QWidget *parent)
        : QWidget(parent), ui(new Ui::FtbPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &FtbPage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    model = new Ftb::ListModel(this);
    ui->packView->setModel(model);
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

                model->getLogo(selected.name, art.url, [this, editedLogoName](QString logo)
                {
                    dialog->setSuggestedIconFromFile(logo + ".small", editedLogoName);
                });
            }
        }
    }
}

void FtbPage::triggerSearch()
{
    model->searchWithTerm(ui->searchEdit->text());
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

    selected = model->data(first, Qt::UserRole).value<ModpacksCH::Modpack>();

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
