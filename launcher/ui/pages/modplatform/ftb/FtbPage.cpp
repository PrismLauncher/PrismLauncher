/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 * Copyright 2021 Philip T <me@phit.link>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FtbPage.h"
#include "ui_FtbPage.h"

#include <QKeyEvent>

#include "ui/dialogs/NewInstanceDialog.h"
#include "modplatform/modpacksch/FTBPackInstallTask.h"

#include "HoeDown.h"

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

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    for(int i = 0; i < filterModel->getAvailableSortings().size(); i++)
    {
        ui->sortByBox->addItem(filterModel->getAvailableSortings().keys().at(i));
    }
    ui->sortByBox->setCurrentText(filterModel->translateCurrentSorting());

    connect(ui->searchEdit, &QLineEdit::textChanged, this, &FtbPage::triggerSearch);
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
    if(!initialised)
    {
        listModel->request();
        initialised = true;
    }

    suggestCurrent();
}

void FtbPage::suggestCurrent()
{
    if(!isOpened)
    {
        return;
    }

    if (selectedVersion.isEmpty())
    {
        dialog->setSuggestedPack();
        return;
    }

    dialog->setSuggestedPack(selected.name + " " + selectedVersion, new ModpacksCH::PackInstallTask(selected, selectedVersion));
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

void FtbPage::triggerSearch()
{
    filterModel->setSearchTerm(ui->searchEdit->text());
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

    HoeDown hoedown;
    QString output = hoedown.process(selected.description.toUtf8());
    ui->packDescription->setHtml(output);

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
