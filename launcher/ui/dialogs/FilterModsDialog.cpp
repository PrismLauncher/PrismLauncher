#include "FilterModsDialog.h"
#include "ui_FilterModsDialog.h"

FilterModsDialog::FilterModsDialog(Version def, QWidget* parent)
    : QDialog(parent), m_filter(new Filter()), ui(new Ui::FilterModsDialog)
{
    ui->setupUi(this);

    m_mcVersion_buttons.addButton(ui->strictVersionButton,   VersionButtonID::Strict);
    m_mcVersion_buttons.addButton(ui->majorVersionButton,    VersionButtonID::Major);
    m_mcVersion_buttons.addButton(ui->allVersionsButton,     VersionButtonID::All);
    m_mcVersion_buttons.addButton(ui->betweenVersionsButton, VersionButtonID::Between);

    connect(&m_mcVersion_buttons, SIGNAL(idClicked(int)), this, SLOT(onVersionFilterChanged(int)));

    m_filter->versions.push_front(def);
}

int FilterModsDialog::execWithInstance(MinecraftInstance* instance)
{
    m_instance = instance;

    // Fix first openening behaviour
    onVersionFilterChanged(0);

    auto mcVersionSplit = mcVersionStr().split(".");

    ui->strictVersionButton->setText(
        tr("Strict match (= %1)").arg(mcVersionStr()));
    ui->majorVersionButton->setText(
        tr("Major varsion match (= %1.%2.x)").arg(mcVersionSplit[0], mcVersionSplit[1]));
    ui->allVersionsButton->setText(
        tr("Any version match"));
    ui->betweenVersionsButton->setText(
        tr("Between two versions"));

    int ret = QDialog::exec();
    m_instance = nullptr;
    return ret;
}

void FilterModsDialog::onVersionFilterChanged(int id)
{
    ui->lowerVersionComboBox->setEnabled(id == VersionButtonID::Between);
    ui->upperVersionComboBox->setEnabled(id == VersionButtonID::Between);

    auto versionSplit = mcVersionStr().split(".");
    int index = 0;

    m_filter->versions.clear();

    switch(id){
    case(VersionButtonID::Strict):
        m_filter->versions.push_front(mcVersion());
        break;
    case(VersionButtonID::Major):
        for(auto i = Version(QString("%1.%2").arg(versionSplit[0], versionSplit[1])); i <= mcVersion(); index++){
            m_filter->versions.push_front(i);
            i = Version(QString("%1.%2.%3").arg(versionSplit[0], versionSplit[1], QString("%1").arg(index)));
        }
        break;
    case(VersionButtonID::All):
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
