#include "ModFilterWidget.h"
#include <qcheckbox.h>
#include <qcombobox.h>
#include "BaseVersionList.h"
#include "meta/Index.h"
#include "modplatform/ModIndex.h"
#include "ui_ModFilterWidget.h"

#include "Application.h"
#include "minecraft/PackProfile.h"

unique_qobject_ptr<ModFilterWidget> ModFilterWidget::create(MinecraftInstance* instance, QWidget* parent)
{
    return unique_qobject_ptr<ModFilterWidget>(new ModFilterWidget(instance, parent));
}

ModFilterWidget::ModFilterWidget(MinecraftInstance* instance, QWidget* parent)
    : QTabWidget(parent), ui(new Ui::ModFilterWidget), m_instance(instance), m_filter(new Filter())
{
    ui->setupUi(this);

    m_versions_proxy = new VersionProxyModel(this);

    ui->versionsCb->setModel(m_versions_proxy);

    m_versions_proxy->setFilter(BaseVersionList::TypeRole, new RegexpFilter("(release)", false));

    ui->versionsCb->setStyleSheet("combobox-popup: 0;");
    connect(ui->snapshotsCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onIncludeSnapshotsChanged);
    connect(ui->versionsCb, &QComboBox::currentIndexChanged, this, &ModFilterWidget::onVersionFilterChanged);

    connect(ui->neoForgeCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->forgeCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->fabricCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->quiltCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->liteLoaderCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->cauldronCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);

    ui->liteLoaderCb->hide();
    ui->cauldronCb->hide();

    connect(ui->serverEnv, &QCheckBox::stateChanged, this, &ModFilterWidget::onSideFilterChanged);
    connect(ui->clientEnv, &QCheckBox::stateChanged, this, &ModFilterWidget::onSideFilterChanged);

    connect(ui->hide_installed, &QCheckBox::stateChanged, this, &ModFilterWidget::onHideInstalledFilterChanged);

    setHidden(true);
    loadVersionList();
    prepareBasicFilter();
}

auto ModFilterWidget::getFilter() -> std::shared_ptr<Filter>
{
    m_filter_changed = false;
    emit filterUnchanged();
    return m_filter;
}

ModFilterWidget::~ModFilterWidget()
{
    delete ui;
}

void ModFilterWidget::loadVersionList()
{
    m_version_list = APPLICATION->metadataIndex()->get("net.minecraft");
    if (!m_version_list->isLoaded()) {
        QEventLoop load_version_list_loop;

        QTimer time_limit_for_list_load;
        time_limit_for_list_load.setTimerType(Qt::TimerType::CoarseTimer);
        time_limit_for_list_load.setSingleShot(true);
        time_limit_for_list_load.callOnTimeout(&load_version_list_loop, &QEventLoop::quit);
        time_limit_for_list_load.start(4000);

        auto task = m_version_list->getLoadTask();

        connect(task.get(), &Task::failed, [this] {
            ui->versionsCb->setEnabled(false);
            ui->snapshotsCb->setEnabled(false);
        });
        connect(task.get(), &Task::finished, &load_version_list_loop, &QEventLoop::quit);

        if (!task->isRunning())
            task->start();

        load_version_list_loop.exec();
        if (time_limit_for_list_load.isActive())
            time_limit_for_list_load.stop();
    }
    m_versions_proxy->setSourceModel(m_version_list.get());
}

void ModFilterWidget::prepareBasicFilter()
{
    m_filter->hideInstalled = false;
    m_filter->side = "";  // or "both"t
    auto loaders = m_instance->getPackProfile()->getSupportedModLoaders().value();
    ui->neoForgeCb->setChecked(loaders & ModPlatform::NeoForge);
    ui->forgeCb->setChecked(loaders & ModPlatform::Forge);
    ui->fabricCb->setChecked(loaders & ModPlatform::Fabric);
    ui->quiltCb->setChecked(loaders & ModPlatform::Quilt);
    ui->liteLoaderCb->setChecked(loaders & ModPlatform::LiteLoader);
    ui->cauldronCb->setChecked(loaders & ModPlatform::Cauldron);
    m_filter->loaders = loaders;
    auto def = m_instance->getPackProfile()->getComponentVersion("net.minecraft");
    m_filter->versions.push_front(Version{ def });
    m_versions_proxy->setCurrentVersion(def);
    ui->versionsCb->setCurrentIndex(m_versions_proxy->getVersion(def).row());
}

void ModFilterWidget::onIncludeSnapshotsChanged()
{
    QString filter = "(release)";
    if (ui->snapshotsCb->isChecked())
        filter += "|(snapshot)";
    m_versions_proxy->setFilter(BaseVersionList::TypeRole, new RegexpFilter(filter, false));
}

void ModFilterWidget::onVersionFilterChanged()
{
    auto version = ui->versionsCb->currentData(BaseVersionList::VersionIdRole).toString();
    m_filter->versions.clear();
    m_filter->versions.push_front(version);
    m_filter_changed = true;
    emit filterChanged();
}

void ModFilterWidget::onLoadersFilterChanged()
{
    ModPlatform::ModLoaderTypes loaders;
    if (ui->neoForgeCb->isChecked())
        loaders |= ModPlatform::NeoForge;
    if (ui->forgeCb->isChecked())
        loaders |= ModPlatform::Forge;
    if (ui->fabricCb->isChecked())
        loaders |= ModPlatform::Fabric;
    if (ui->quiltCb->isChecked())
        loaders |= ModPlatform::Quilt;
    if (ui->cauldronCb->isChecked())
        loaders |= ModPlatform::Cauldron;
    if (ui->liteLoaderCb->isChecked())
        loaders |= ModPlatform::LiteLoader;
    m_filter_changed = loaders != m_filter->loaders;
    m_filter->loaders = loaders;
    if (m_filter_changed)
        emit filterChanged();
    else
        emit filterUnchanged();
}

void ModFilterWidget::onSideFilterChanged()
{
    QString side;
    if (ui->serverEnv->isChecked())
        side = "server";
    if (ui->clientEnv->isChecked()) {
        if (side.isEmpty())
            side = "client";
        else
            side = "";  // or both
    }
    m_filter_changed = side != m_filter->side;
    m_filter->side = side;
    if (m_filter_changed)
        emit filterChanged();
    else
        emit filterUnchanged();
}

void ModFilterWidget::onHideInstalledFilterChanged()
{
    auto hide = ui->hide_installed->isChecked();
    m_filter_changed = hide != m_filter->hideInstalled;
    m_filter->hideInstalled = hide;
    if (m_filter_changed)
        emit filterChanged();
    else
        emit filterUnchanged();
}