#include "ModFilterWidget.h"
#include "ui_ModFilterWidget.h"

#include "Application.h"

unique_qobject_ptr<ModFilterWidget> ModFilterWidget::create(Version default_version, QWidget* parent)
{
    auto filter_widget = new ModFilterWidget(default_version, parent);

    if (!filter_widget->versionList()->isLoaded()) {
        QEventLoop load_version_list_loop;

        QTimer time_limit_for_list_load;
        time_limit_for_list_load.setTimerType(Qt::TimerType::CoarseTimer);
        time_limit_for_list_load.setSingleShot(true);
        time_limit_for_list_load.callOnTimeout(&load_version_list_loop, &QEventLoop::quit);
        time_limit_for_list_load.start(4000);

        auto task = filter_widget->versionList()->getLoadTask();

        connect(task.get(), &Task::failed, [filter_widget]{
            filter_widget->disableVersionButton(VersionButtonID::Major, tr("failed to get version index"));
        });
        connect(task.get(), &Task::finished, &load_version_list_loop, &QEventLoop::quit);

        if (!task->isRunning())
            task->start();

        load_version_list_loop.exec();
        if (time_limit_for_list_load.isActive())
            time_limit_for_list_load.stop();
    }

    return unique_qobject_ptr<ModFilterWidget>(filter_widget);
}

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

    m_version_list = APPLICATION->metadataIndex()->get("net.minecraft");
    setHidden(true);
}

void ModFilterWidget::setInstance(MinecraftInstance* instance)
{
    m_instance = instance;

    ui->strictVersionButton->setText(
        tr("Strict match (= %1)").arg(mcVersionStr()));

    // we can't do this for snapshots sadly
    if(mcVersionStr().contains('.'))
    {
        auto mcVersionSplit = mcVersionStr().split(".");
        ui->majorVersionButton->setText(
            tr("Major version match (= %1.%2.x)").arg(mcVersionSplit[0], mcVersionSplit[1]));
    }
    else
    {
        ui->majorVersionButton->setText(tr("Major version match (unsupported)"));
        disableVersionButton(Major);
    }
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

void ModFilterWidget::disableVersionButton(VersionButtonID id, QString reason)
{
    QAbstractButton* btn = nullptr;

    switch(id){
    case(VersionButtonID::Strict):
        btn = ui->strictVersionButton;
        break;
    case(VersionButtonID::Major):
        btn = ui->majorVersionButton;
        break;
    case(VersionButtonID::All):
        btn = ui->allVersionsButton;
        break;
    case(VersionButtonID::Between):
    default:
        break;
    }

    if (btn) {
        btn->setEnabled(false);
        if (!reason.isEmpty())
            btn->setText(btn->text() + QString(" (%1)").arg(reason));
    }
}

void ModFilterWidget::onVersionFilterChanged(int id)
{
    //ui->lowerVersionComboBox->setEnabled(id == VersionButtonID::Between);
    //ui->upperVersionComboBox->setEnabled(id == VersionButtonID::Between);

    int index = 1;

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
    case(VersionButtonID::Major): {
        auto versionSplit = mcVersionStr().split(".");

        auto major_version = QString("%1.%2").arg(versionSplit[0], versionSplit[1]);
        QString version_str = major_version;

        while (m_version_list->hasVersion(version_str)) {
            m_filter->versions.emplace_back(version_str);
            version_str = QString("%1.%2").arg(major_version, QString::number(index++));
        }

        break;
    }
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
