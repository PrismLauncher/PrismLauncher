// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ModFilterWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <algorithm>
#include "BaseVersionList.h"
#include "meta/Index.h"
#include "modplatform/ModIndex.h"
#include "ui_ModFilterWidget.h"

#include "Application.h"
#include "minecraft/PackProfile.h"

unique_qobject_ptr<ModFilterWidget> ModFilterWidget::create(MinecraftInstance* instance, bool extendedSupport, QWidget* parent)
{
    return unique_qobject_ptr<ModFilterWidget>(new ModFilterWidget(instance, extendedSupport, parent));
}

class VersionBasicModel : public QIdentityProxyModel {
    Q_OBJECT

   public:
    explicit VersionBasicModel(QObject* parent = nullptr) : QIdentityProxyModel(parent) {}

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole)
            return QIdentityProxyModel::data(index, BaseVersionList::VersionIdRole);
        return {};
    }
};

ModFilterWidget::ModFilterWidget(MinecraftInstance* instance, bool extendedSupport, QWidget* parent)
    : QTabWidget(parent), ui(new Ui::ModFilterWidget), m_instance(instance), m_filter(new Filter())
{
    ui->setupUi(this);

    m_versions_proxy = new VersionProxyModel(this);
    m_versions_proxy->setFilter(BaseVersionList::TypeRole, new RegexpFilter("(release)", false));

    auto proxy = new VersionBasicModel(this);
    proxy->setSourceModel(m_versions_proxy);

    if (!extendedSupport) {
        ui->versionsSimpleCb->setModel(proxy);
        ui->versionsCb->hide();
        ui->snapshotsCb->hide();
        ui->envBox->hide();
    } else {
        ui->versionsCb->setSourceModel(proxy);
        ui->versionsCb->setSeparator("| ");
        ui->versionsSimpleCb->hide();
    }

    ui->versionsCb->setStyleSheet("combobox-popup: 0;");
    ui->versionsSimpleCb->setStyleSheet("combobox-popup: 0;");
    connect(ui->snapshotsCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onIncludeSnapshotsChanged);
    connect(ui->versionsCb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModFilterWidget::onVersionFilterChanged);
    connect(ui->versionsSimpleCb, &QComboBox::currentTextChanged, this, &ModFilterWidget::onVersionFilterTextChanged);

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

    connect(ui->releaseCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(ui->betaCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(ui->alphaCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(ui->unknownCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onReleaseFilterChanged);

    connect(ui->categoriesList, &QListWidget::itemClicked, this, &ModFilterWidget::onCategoryClicked);

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
    ui->versionsCb->setCheckedItems({ def });
}

void ModFilterWidget::onIncludeSnapshotsChanged()
{
    QString filter = "(release)";
    if (ui->snapshotsCb->isChecked())
        filter += "|(snapshot)";
    m_versions_proxy->setFilter(BaseVersionList::TypeRole, new RegexpFilter(filter, false));
}

void ModFilterWidget::onVersionFilterChanged(int)
{
    auto versions = ui->versionsCb->checkedItems();
    m_filter->versions.clear();
    for (auto version : versions)
        m_filter->versions.push_back(version);
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

void ModFilterWidget::onReleaseFilterChanged()
{
    std::list<ModPlatform::IndexedVersionType> releases;
    if (ui->releaseCb->isChecked())
        releases.push_back(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Release));
    if (ui->betaCb->isChecked())
        releases.push_back(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Beta));
    if (ui->alphaCb->isChecked())
        releases.push_back(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Alpha));
    if (ui->unknownCb->isChecked())
        releases.push_back(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Unknown));
    m_filter_changed = releases != m_filter->releases;
    m_filter->releases = releases;
    if (m_filter_changed)
        emit filterChanged();
    else
        emit filterUnchanged();
}

void ModFilterWidget::onVersionFilterTextChanged(QString version)
{
    m_filter->versions.clear();
    m_filter->versions.push_front(version);
    m_filter_changed = true;
    emit filterChanged();
}

void ModFilterWidget::setCategories(QList<ModPlatform::Category> categories)
{
    ui->categoriesList->clear();
    m_categories = categories;
    for (auto cat : categories) {
        auto item = new QListWidgetItem(cat.name, ui->categoriesList);
        item->setFlags(item->flags() & (~Qt::ItemIsUserCheckable));
        item->setCheckState(Qt::Unchecked);
        ui->categoriesList->addItem(item);
    }
}

void ModFilterWidget::onCategoryClicked(QListWidgetItem* item)
{
    if (item)
        item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    m_filter->categoryIds.clear();
    for (auto i = 0; i < ui->categoriesList->count(); i++) {
        auto item = ui->categoriesList->item(i);
        if (item->checkState() == Qt::Checked) {
            auto c = std::find_if(m_categories.cbegin(), m_categories.cend(), [item](auto v) { return v.name == item->text(); });
            if (c != m_categories.cend())
                m_filter->categoryIds << c->id;
        }
    }
    m_filter_changed = true;
    emit filterChanged();
};

#include "ModFilterWidget.moc"