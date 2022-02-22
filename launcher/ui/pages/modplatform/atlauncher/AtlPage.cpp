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

#include "AtlPage.h"
#include "ui_AtlPage.h"

#include "modplatform/atlauncher/ATLPackInstallTask.h"

#include "AtlOptionalModDialog.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/dialogs/VersionSelectDialog.h"

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

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    for(int i = 0; i < filterModel->getAvailableSortings().size(); i++)
    {
        ui->sortByBox->addItem(filterModel->getAvailableSortings().keys().at(i));
    }
    ui->sortByBox->setCurrentText(filterModel->translateCurrentSorting());

    connect(ui->searchEdit, &QLineEdit::textChanged, this, &AtlPage::triggerSearch);
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

void AtlPage::retranslate()
{
    ui->retranslateUi(this);
}

void AtlPage::openedImpl()
{
    if(!initialized)
    {
        listModel->request();
        initialized = true;
    }

    suggestCurrent();
}

void AtlPage::suggestCurrent()
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

    dialog->setSuggestedPack(selected.name + " " + selectedVersion, new ATLauncher::PackInstallTask(this, selected.safeName, selectedVersion));
    auto editedLogoName = selected.safeName;
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/images/%1.png").arg(selected.safeName.toLower());
    listModel->getLogo(selected.safeName, url, [this, editedLogoName](QString logo)
    {
        dialog->setSuggestedIconFromFile(logo, editedLogoName);
    });
}

void AtlPage::triggerSearch()
{
    filterModel->setSearchTerm(ui->searchEdit->text());
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

QVector<QString> AtlPage::chooseOptionalMods(QVector<ATLauncher::VersionMod> mods) {
    AtlOptionalModDialog optionalModDialog(this, mods);
    optionalModDialog.exec();
    return optionalModDialog.getResult();
}

QString AtlPage::chooseVersion(Meta::VersionListPtr vlist, QString minecraftVersion) {
    VersionSelectDialog vselect(vlist.get(), "Choose Version", APPLICATION->activeWindow(), false);
    if (minecraftVersion != Q_NULLPTR) {
        vselect.setExactFilter(BaseVersionList::ParentVersionRole, minecraftVersion);
        vselect.setEmptyString(tr("No versions are currently available for Minecraft %1").arg(minecraftVersion));
    }
    else {
        vselect.setEmptyString(tr("No versions are currently available"));
    }
    vselect.setEmptyErrorString(tr("Couldn't load or download the version lists!"));

    // select recommended build
    for (int i = 0; i < vlist->versions().size(); i++) {
        auto version = vlist->versions().at(i);
        auto reqs = version->requires();

        // filter by minecraft version, if the loader depends on a certain version.
        if (minecraftVersion != Q_NULLPTR) {
            auto iter = std::find_if(reqs.begin(), reqs.end(), [](const Meta::Require &req) {
                return req.uid == "net.minecraft";
            });
            if (iter == reqs.end()) continue;
            if (iter->equalsVersion != minecraftVersion) continue;
        }

        // first recommended build we find, we use.
        if (version->isRecommended()) {
            vselect.setCurrentVersion(version->descriptor());
            break;
        }
    }

    vselect.exec();
    return vselect.selectedVersion()->descriptor();
}
