#include "ModDownloadDialog.h"

#include <BaseVersion.h>
#include <icons/IconList.h>
#include <InstanceList.h>

#include "ProgressDialog.h"
#include "ReviewMessageBox.h"

#include <QLayout>
#include <QPushButton>
#include <QValidator>
#include <QDialogButtonBox>

#include "ui/widgets/PageContainer.h"
#include "ui/pages/modplatform/modrinth/ModrinthModPage.h"
#include "ModDownloadTask.h"


ModDownloadDialog::ModDownloadDialog(const std::shared_ptr<ModFolderModel> &mods, QWidget *parent,
                                     BaseInstance *instance)
    : QDialog(parent), mods(mods), m_instance(instance)
{
    setObjectName(QStringLiteral("ModDownloadDialog"));

    resize(std::max(0.5*parent->width(), 400.0), std::max(0.75*parent->height(), 400.0));

    m_verticalLayout = new QVBoxLayout(this);
    m_verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    setWindowIcon(APPLICATION->getThemedIcon("new"));
    // NOTE: m_buttons must be initialized before PageContainer, because it indirectly accesses m_buttons through setSuggestedPack! Do not move this below.
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    m_container = new PageContainer(this);
    m_container->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
    m_container->layout()->setContentsMargins(0, 0, 0, 0);
    m_verticalLayout->addWidget(m_container);

    m_container->addButtons(m_buttons);

    // Bonk Qt over its stupid head and make sure it understands which button is the default one...
    // See: https://stackoverflow.com/questions/24556831/qbuttonbox-set-default-button
    auto OkButton = m_buttons->button(QDialogButtonBox::Ok);
    OkButton->setEnabled(false);
    OkButton->setDefault(true);
    OkButton->setAutoDefault(true);
    connect(OkButton, &QPushButton::clicked, this, &ModDownloadDialog::confirm);

    auto CancelButton = m_buttons->button(QDialogButtonBox::Cancel);
    CancelButton->setDefault(false);
    CancelButton->setAutoDefault(false);
    connect(CancelButton, &QPushButton::clicked, this, &ModDownloadDialog::reject);

    auto HelpButton = m_buttons->button(QDialogButtonBox::Help);
    HelpButton->setDefault(false);
    HelpButton->setAutoDefault(false);
    connect(HelpButton, &QPushButton::clicked, m_container, &PageContainer::help);

    QMetaObject::connectSlotsByName(this);
    setWindowModality(Qt::WindowModal);
    setWindowTitle("Download mods");
}

QString ModDownloadDialog::dialogTitle()
{
    return tr("Download mods");
}

void ModDownloadDialog::reject()
{
    QDialog::reject();
}

void ModDownloadDialog::confirm()
{
    auto keys = modTask.keys();
    keys.sort(Qt::CaseInsensitive);

    auto confirm_dialog = ReviewMessageBox::create(
        this,
        tr("Confirm mods to download")
    );

    for(auto& task : keys){
        confirm_dialog->appendMod(task, modTask.find(task).value()->getFilename());
    }

    connect(confirm_dialog, &QDialog::accepted, this, &ModDownloadDialog::accept);

    confirm_dialog->open();
}

void ModDownloadDialog::accept()
{
    QDialog::accept();
}

QList<BasePage *> ModDownloadDialog::getPages()
{
    modrinthPage = new ModrinthModPage(this, m_instance);
    flameModPage = new FlameModPage(this, m_instance);
    return
    {
        modrinthPage,
        flameModPage
    };
}

void ModDownloadDialog::addSelectedMod(const QString& name, ModDownloadTask* task)
{
    removeSelectedMod(name);
    modTask.insert(name, task);

    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!modTask.isEmpty());
}

void ModDownloadDialog::removeSelectedMod(const QString &name)
{
    if(modTask.contains(name))
        delete modTask.find(name).value();
    modTask.remove(name);

    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!modTask.isEmpty());
}

bool ModDownloadDialog::isModSelected(const QString &name, const QString& filename) const
{
    // FIXME: Is there a way to check for versions without checking the filename
    //        as a heuristic, other than adding such info to ModDownloadTask itself?
    auto iter = modTask.find(name);
    return iter != modTask.end() && (iter.value()->getFilename() == filename);
}

ModDownloadDialog::~ModDownloadDialog()
{
}

const QList<ModDownloadTask*> ModDownloadDialog::getTasks() {
    return modTask.values();
}
