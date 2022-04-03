#include "FilterModsDialog.h"
#include "ui_FilterModsDialog.h"

FilterModsDialog::FilterModsDialog(Version def, QWidget* parent)
    : QDialog(parent), m_filter(new Filter()), m_internal_filter(new Filter()), ui(new Ui::FilterModsDialog)
{
    ui->setupUi(this);

    m_mcVersion_buttons.addButton(ui->strictVersionButton,   VersionButtonID::Strict);
    m_mcVersion_buttons.addButton(ui->majorVersionButton,    VersionButtonID::Major);
    m_mcVersion_buttons.addButton(ui->allVersionsButton,     VersionButtonID::All);
    //m_mcVersion_buttons.addButton(ui->betweenVersionsButton, VersionButtonID::Between);

    connect(&m_mcVersion_buttons, SIGNAL(idClicked(int)), this, SLOT(onVersionFilterChanged(int)));

    m_internal_filter->versions.push_front(def);
    commitChanges();
}

int FilterModsDialog::execWithInstance(MinecraftInstance* instance)
{
    m_instance = instance;

    auto* pressed_button = m_mcVersion_buttons.checkedButton();

    // Fix first openening behaviour
    onVersionFilterChanged(m_previous_mcVersion_id);

    auto mcVersionSplit = mcVersionStr().split(".");

    ui->strictVersionButton->setText(
        tr("Strict match (= %1)").arg(mcVersionStr()));
    ui->majorVersionButton->setText(
        tr("Major version match (= %1.%2.x)").arg(mcVersionSplit[0], mcVersionSplit[1]));
    ui->allVersionsButton->setText(
        tr("Any version"));
    //ui->betweenVersionsButton->setText(
    //    tr("Between two versions"));

    int ret = QDialog::exec();

    if(ret == QDialog::DialogCode::Accepted){
        // If there's no change, let's sey it's a cancel to the caller
        if(*m_internal_filter.get() == *m_filter.get())
            return QDialog::DialogCode::Rejected;

        m_previous_mcVersion_id = (VersionButtonID) m_mcVersion_buttons.checkedId();
        commitChanges();
    } else {
        pressed_button->click();
        revertChanges();
    }

    m_instance = nullptr;
    return ret;
}

void FilterModsDialog::disableVersionButton(VersionButtonID id)
{
    switch(id){
    case(VersionButtonID::Strict):
        ui->strictVersionButton->setEnabled(false);
        break;
    case(VersionButtonID::Major):
        ui->majorVersionButton->setEnabled(false);
        break;
    case(VersionButtonID::All):
        ui->allVersionsButton->setEnabled(false);
        break;
    case(VersionButtonID::Between):
    //    ui->betweenVersionsButton->setEnabled(false);
        break;
    default:
        break;
    }
}

// Do deep copy
void FilterModsDialog::commitChanges()
{
    m_filter->versions = m_internal_filter->versions;
}
void FilterModsDialog::revertChanges()
{
    m_internal_filter->versions = m_filter->versions;
}

void FilterModsDialog::onVersionFilterChanged(int id)
{
    //ui->lowerVersionComboBox->setEnabled(id == VersionButtonID::Between);
    //ui->upperVersionComboBox->setEnabled(id == VersionButtonID::Between);

    auto versionSplit = mcVersionStr().split(".");
    int index = 0;

    m_internal_filter->versions.clear();

    switch(id){
    case(VersionButtonID::Strict):
        m_internal_filter->versions.push_front(mcVersion());
        break;
    case(VersionButtonID::Major):
        for(auto i = Version(QString("%1.%2").arg(versionSplit[0], versionSplit[1])); i <= mcVersion(); index++){
            m_internal_filter->versions.push_front(i);
            i = Version(QString("%1.%2.%3").arg(versionSplit[0], versionSplit[1], QString("%1").arg(index)));
        }
        break;
    case(VersionButtonID::All):
        // Empty list to avoid enumerating all versions :P
        break;
    case(VersionButtonID::Between):
        // TODO
        break;
    default:
        break;
    }
}

FilterModsDialog::~FilterModsDialog()
{
    delete ui;
}
