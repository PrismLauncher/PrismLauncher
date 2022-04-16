#include "ModFilterWidget.h"
#include "ui_ModFilterWidget.h"

ModFilterWidget::ModFilterWidget(Version def, QWidget* parent)
    : QTabWidget(parent), m_filter(new Filter()),  ui(new Ui::ModFilterWidget)
{
    ui->setupUi(this);

    m_mcVersion_buttons.addButton(ui->strictVersionButton,   VersionButtonID::Strict);
    ui->strictVersionButton->click();
    m_mcVersion_buttons.addButton(ui->majorVersionButton,    VersionButtonID::Major);
    m_mcVersion_buttons.addButton(ui->allVersionsButton,     VersionButtonID::All);
    //m_mcVersion_buttons.addButton(ui->betweenVersionsButton, VersionButtonID::Between);

    connect(&m_mcVersion_buttons, SIGNAL(idClicked(int)), this, SLOT(onVersionFilterChanged(int)));

    m_filter->versions.push_front(def);

    setHidden(true);
}

void ModFilterWidget::setInstance(MinecraftInstance* instance)
{
    m_instance = instance;

    auto mcVersionSplit = mcVersionStr().split(".");

    ui->strictVersionButton->setText(
        tr("Strict match (= %1)").arg(mcVersionStr()));
    ui->majorVersionButton->setText(
        tr("Major version match (= %1.%2.x)").arg(mcVersionSplit[0], mcVersionSplit[1]));
    ui->allVersionsButton->setText(
        tr("Any version"));
    //ui->betweenVersionsButton->setText(
    //    tr("Between two versions"));
}

auto ModFilterWidget::getFilter() -> std::shared_ptr<Filter>
{
    m_last_version_id = m_version_id;
    emit filterUnchanged();
    return m_filter;
}

void ModFilterWidget::disableVersionButton(VersionButtonID id)
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

void ModFilterWidget::onVersionFilterChanged(int id)
{
    //ui->lowerVersionComboBox->setEnabled(id == VersionButtonID::Between);
    //ui->upperVersionComboBox->setEnabled(id == VersionButtonID::Between);

    auto versionSplit = mcVersionStr().split(".");
    int index = 0;

    auto cast_id = (VersionButtonID) id;
    if (cast_id != m_version_id) {
        m_version_id = cast_id;
    } else {
        return;
    }

    m_filter->versions.clear();

    switch(cast_id){
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
        // Empty list to avoid enumerating all versions :P
        break;
    case(VersionButtonID::Between):
        // TODO
        break;
    }

    if(changed())
        emit filterChanged();
    else
        emit filterUnchanged();
}

ModFilterWidget::~ModFilterWidget()
{
    delete ui;
}
