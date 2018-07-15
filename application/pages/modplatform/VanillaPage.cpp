#include "VanillaPage.h"
#include "ui_VanillaPage.h"

#include "MultiMC.h"
#include "FolderInstanceProvider.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/ProgressDialog.h"

#include <meta/Index.h>
#include <meta/VersionList.h>
#include <dialogs/NewInstanceDialog.h>
#include <Filter.h>
#include <Env.h>
#include <InstanceCreationTask.h>
#include <QTabBar>

VanillaPage::VanillaPage(NewInstanceDialog *dialog, QWidget *parent)
    : QWidget(parent), dialog(dialog), ui(new Ui::VanillaPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    connect(ui->versionList, &VersionSelectWidget::selectedVersionChanged, this, &VanillaPage::setSelectedVersion);
    filterChanged();
    connect(ui->alphaFilter, &QCheckBox::stateChanged, this, &VanillaPage::filterChanged);
    connect(ui->betaFilter, &QCheckBox::stateChanged, this, &VanillaPage::filterChanged);
    connect(ui->snapshotFilter, &QCheckBox::stateChanged, this, &VanillaPage::filterChanged);
    connect(ui->oldSnapshotFilter, &QCheckBox::stateChanged, this, &VanillaPage::filterChanged);
    connect(ui->releaseFilter, &QCheckBox::stateChanged, this, &VanillaPage::filterChanged);
    connect(ui->refreshBtn, &QPushButton::clicked, this, &VanillaPage::refresh);
}

void VanillaPage::openedImpl()
{
    if(!initialized)
    {
        auto vlist = ENV.metadataIndex()->get("net.minecraft");
        ui->versionList->initialize(vlist.get());
        initialized = true;
    }
    else
    {
        suggestCurrent();
    }
}

void VanillaPage::refresh()
{
    ui->versionList->loadList();
}

void VanillaPage::filterChanged()
{
    QStringList out;
    if(ui->alphaFilter->isChecked())
        out << "(old_alpha)";
    if(ui->betaFilter->isChecked())
        out << "(old_beta)";
    if(ui->snapshotFilter->isChecked())
        out << "(snapshot)";
    if(ui->oldSnapshotFilter->isChecked())
        out << "(old_snapshot)";
    if(ui->releaseFilter->isChecked())
        out << "(release)";
    auto regexp = out.join('|');
    ui->versionList->setFilter(BaseVersionList::TypeRole, new RegexpFilter(regexp, false));
}

VanillaPage::~VanillaPage()
{
    delete ui;
}

bool VanillaPage::shouldDisplay() const
{
    return true;
}

BaseVersionPtr VanillaPage::selectedVersion() const
{
    return m_selectedVersion;
}

void VanillaPage::suggestCurrent()
{
    if(m_selectedVersion && isOpened)
    {
        dialog->setSuggestedPack(m_selectedVersion->descriptor(), new InstanceCreationTask(m_selectedVersion));
    }
}

void VanillaPage::setSelectedVersion(BaseVersionPtr version)
{
    m_selectedVersion = version;
    suggestCurrent();
}
