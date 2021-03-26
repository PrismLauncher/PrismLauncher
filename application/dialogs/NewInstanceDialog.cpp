/* Copyright 2013-2021 MultiMC Contributors
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

#include "MultiMC.h"
#include "NewInstanceDialog.h"
#include "ui_NewInstanceDialog.h"

#include <BaseVersion.h>
#include <icons/IconList.h>
#include <tasks/Task.h>
#include <InstanceList.h>

#include "VersionSelectDialog.h"
#include "ProgressDialog.h"
#include "IconPickerDialog.h"

#include <QLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QValidator>
#include <QDialogButtonBox>

#include "widgets/PageContainer.h"
#include <pages/modplatform/VanillaPage.h>
#include <pages/modplatform/atlauncher/AtlPage.h>
#include <pages/modplatform/ftb/FtbPage.h>
#include <pages/modplatform/legacy_ftb/Page.h>
#include <pages/modplatform/flame/FlamePage.h>
#include <pages/modplatform/ImportPage.h>
#include <pages/modplatform/technic/TechnicPage.h>



NewInstanceDialog::NewInstanceDialog(const QString & initialGroup, const QString & url, QWidget *parent)
    : QDialog(parent), ui(new Ui::NewInstanceDialog)
{
    ui->setupUi(this);

    setWindowIcon(MMC->getThemedIcon("new"));

    InstIconKey = "default";
    ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));

    auto groups = MMC->instances()->getGroups().toSet();
    auto groupList = QStringList(groups.toList());
    groupList.sort(Qt::CaseInsensitive);
    groupList.removeOne("");
    groupList.push_front(initialGroup);
    groupList.push_front("");
    ui->groupBox->addItems(groupList);
    int index = groupList.indexOf(initialGroup);
    if(index == -1)
    {
        index = 0;
    }
    ui->groupBox->setCurrentIndex(index);
    ui->groupBox->lineEdit()->setPlaceholderText(tr("No group"));


    // NOTE: m_buttons must be initialized before PageContainer, because it indirectly accesses m_buttons through setSuggestedPack! Do not move this below.
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    m_container = new PageContainer(this);
    m_container->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
    m_container->layout()->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->insertWidget(2, m_container);

    m_container->addButtons(m_buttons);

    // Bonk Qt over its stupid head and make sure it understands which button is the default one...
    // See: https://stackoverflow.com/questions/24556831/qbuttonbox-set-default-button
    auto OkButton = m_buttons->button(QDialogButtonBox::Ok);
    OkButton->setDefault(true);
    OkButton->setAutoDefault(true);
    connect(OkButton, &QPushButton::clicked, this, &NewInstanceDialog::accept);

    auto CancelButton = m_buttons->button(QDialogButtonBox::Cancel);
    CancelButton->setDefault(false);
    CancelButton->setAutoDefault(false);
    connect(CancelButton, &QPushButton::clicked, this, &NewInstanceDialog::reject);

    auto HelpButton = m_buttons->button(QDialogButtonBox::Help);
    HelpButton->setDefault(false);
    HelpButton->setAutoDefault(false);
    connect(HelpButton, &QPushButton::clicked, m_container, &PageContainer::help);

    if(!url.isEmpty())
    {
        QUrl actualUrl(url);
        m_container->selectPage("import");
        importPage->setUrl(url);
    }

    updateDialogState();

    restoreGeometry(QByteArray::fromBase64(MMC->settings()->get("NewInstanceGeometry").toByteArray()));
}

void NewInstanceDialog::reject()
{
    MMC->settings()->set("NewInstanceGeometry", saveGeometry().toBase64());
    QDialog::reject();
}

void NewInstanceDialog::accept()
{
    MMC->settings()->set("NewInstanceGeometry", saveGeometry().toBase64());
    importIconNow();
    QDialog::accept();
}

QList<BasePage *> NewInstanceDialog::getPages()
{
    importPage = new ImportPage(this);
    flamePage = new FlamePage(this);
    auto technicPage = new TechnicPage(this);
    return
    {
        new VanillaPage(this),
        importPage,
        new AtlPage(this),
        flamePage,
        new FtbPage(this),
        new LegacyFTB::Page(this),
        technicPage
    };
}

QString NewInstanceDialog::dialogTitle()
{
    return tr("New Instance");
}

NewInstanceDialog::~NewInstanceDialog()
{
    delete ui;
}

void NewInstanceDialog::setSuggestedPack(const QString& name, InstanceTask* task)
{
    creationTask.reset(task);
    ui->instNameTextBox->setPlaceholderText(name);

    if(!task)
    {
        ui->iconButton->setIcon(MMC->icons()->getIcon("default"));
        importIcon = false;
    }

    auto allowOK = task && !instName().isEmpty();
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(allowOK);
}

void NewInstanceDialog::setSuggestedIconFromFile(const QString &path, const QString &name)
{
    importIcon = true;
    importIconPath = path;
    importIconName = name;

    //Hmm, for some reason they can be to small
    ui->iconButton->setIcon(QIcon(path));
}

InstanceTask * NewInstanceDialog::extractTask()
{
    InstanceTask * extracted = creationTask.get();
    creationTask.release();
    extracted->setName(instName());
    extracted->setGroup(instGroup());
    extracted->setIcon(iconKey());
    return extracted;
}

void NewInstanceDialog::updateDialogState()
{
    auto allowOK = creationTask && !instName().isEmpty();
    auto OkButton = m_buttons->button(QDialogButtonBox::Ok);
    if(OkButton->isEnabled() != allowOK)
    {
        OkButton->setEnabled(allowOK);
    }
}

QString NewInstanceDialog::instName() const
{
    auto result = ui->instNameTextBox->text().trimmed();
    if(result.size())
    {
        return result;
    }
    result = ui->instNameTextBox->placeholderText().trimmed();
    if(result.size())
    {
        return result;
    }
    return QString();
}

QString NewInstanceDialog::instGroup() const
{
    return ui->groupBox->currentText();
}
QString NewInstanceDialog::iconKey() const
{
    return InstIconKey;
}

void NewInstanceDialog::on_iconButton_clicked()
{
    importIconNow(); //so the user can switch back
    IconPickerDialog dlg(this);
    dlg.execWithSelection(InstIconKey);

    if (dlg.result() == QDialog::Accepted)
    {
        InstIconKey = dlg.selectedIconKey;
        ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));
        importIcon = false;
    }
}

void NewInstanceDialog::on_instNameTextBox_textChanged(const QString &arg1)
{
    updateDialogState();
}

void NewInstanceDialog::importIconNow()
{
    if(importIcon) {
        MMC->icons()->installIcon(importIconPath, importIconName);
        InstIconKey = importIconName;
        importIcon = false;
    }
    MMC->settings()->set("NewInstanceGeometry", saveGeometry().toBase64());
}
