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
#include <list>
#include "BaseVersionList.h"
#include "Version.h"
#include "meta/Index.h"
#include "modplatform/ModIndex.h"
#include "ui/widgets/CheckComboBox.h"
#include "ui_ModFilterWidget.h"

#include "Application.h"
#include "minecraft/PackProfile.h"

unique_qobject_ptr<ModFilterWidget> ModFilterWidget::create(MinecraftInstance* instance, bool extended, QWidget* parent)
{
    return unique_qobject_ptr<ModFilterWidget>(new ModFilterWidget(instance, extended, parent));
}

class VersionBasicModel : public QIdentityProxyModel {
    Q_OBJECT

   public:
    explicit VersionBasicModel(QObject* parent = nullptr) : QIdentityProxyModel(parent) {}

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole)
            return QIdentityProxyModel::data(index, BaseVersionList::VersionIdRole);
        if (role == Qt::UserRole)
            return QIdentityProxyModel::data(index, BaseVersionList::VersionIdRole);
        return {};
    }
};

class AllVersionProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

   public:
    AllVersionProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    int rowCount(const QModelIndex& parent = QModelIndex()) const override { return QSortFilterProxyModel::rowCount(parent) + 1; }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid()) {
            return {};
        }

        if (index.row() == 0) {
            if (role == Qt::DisplayRole) {
                return tr("All Versions");
            }
            if (role == Qt::UserRole) {
                return "all";
            }
            return {};
        }

        QModelIndex newIndex = QSortFilterProxyModel::index(index.row() - 1, index.column());
        return QSortFilterProxyModel::data(newIndex, role);
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (index.row() == 0) {
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        }
        return QSortFilterProxyModel::flags(index);
    }
};

ModFilterWidget::ModFilterWidget(MinecraftInstance* instance, bool extended, QWidget* parent)
    : QTabWidget(parent), ui(new Ui::ModFilterWidget), m_instance(instance), m_filter(new Filter())
{
    ui->setupUi(this);

    m_versions_proxy = new VersionProxyModel(this);
    m_versions_proxy->setFilter(BaseVersionList::TypeRole, new ExactFilter("release"));

    QAbstractProxyModel* proxy = new VersionBasicModel(this);
    proxy->setSourceModel(m_versions_proxy);

    if (extended) {
        if (!m_instance) {
            ui->environmentGroup->hide();
        }
        ui->versions->setSourceModel(proxy);
        ui->versions->setSeparator(", ");
        ui->versions->setDefaultText(tr("All Versions"));
        ui->version->hide();
    } else {
        auto allVersions = new AllVersionProxyModel(this);
        allVersions->setSourceModel(proxy);
        proxy = allVersions;
        ui->version->setModel(proxy);
        ui->versions->hide();
        ui->showAllVersions->hide();
        ui->environmentGroup->hide();
    }

    ui->versions->setStyleSheet("combobox-popup: 0;");
    ui->version->setStyleSheet("combobox-popup: 0;");
    connect(ui->showAllVersions, &QCheckBox::stateChanged, this, &ModFilterWidget::onShowAllVersionsChanged);
    connect(ui->versions, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModFilterWidget::onVersionFilterChanged);
    connect(ui->versions, &CheckComboBox::checkedItemsChanged, this, [this] { onVersionFilterChanged(0); });
    connect(ui->version, &QComboBox::currentTextChanged, this, &ModFilterWidget::onVersionFilterTextChanged);

    connect(ui->neoForge, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->forge, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->fabric, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->quilt, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);

    connect(ui->neoForge, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->forge, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->fabric, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(ui->quilt, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);

    if (extended) {
        connect(ui->clientSide, &QCheckBox::stateChanged, this, &ModFilterWidget::onSideFilterChanged);
        connect(ui->serverSide, &QCheckBox::stateChanged, this, &ModFilterWidget::onSideFilterChanged);
    }

    connect(ui->hideInstalled, &QCheckBox::stateChanged, this, &ModFilterWidget::onHideInstalledFilterChanged);

    setHidden(true);
    loadVersionList();
    prepareBasicFilter();
}

auto ModFilterWidget::getFilter() -> std::shared_ptr<Filter>
{
    m_filter_changed = false;
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
            ui->versions->setEnabled(false);
            ui->showAllVersions->setEnabled(false);
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
    if (m_instance) {
        m_filter->hideInstalled = false;
        m_filter->side = "";  // or "both"
        auto loaders = m_instance->getPackProfile()->getSupportedModLoaders().value();
        ui->neoForge->setChecked(loaders & ModPlatform::NeoForge);
        ui->forge->setChecked(loaders & ModPlatform::Forge);
        ui->fabric->setChecked(loaders & ModPlatform::Fabric);
        ui->quilt->setChecked(loaders & ModPlatform::Quilt);
        m_filter->loaders = loaders;
        auto def = m_instance->getPackProfile()->getComponentVersion("net.minecraft");
        m_filter->versions.emplace_front(def);
        ui->versions->setCheckedItems({ def });
        ui->version->setCurrentIndex(ui->version->findText(def));
    } else {
        ui->hideInstalled->hide();
    }
}

void ModFilterWidget::onShowAllVersionsChanged()
{
    if (ui->showAllVersions->isChecked())
        m_versions_proxy->clearFilters();
    else
        m_versions_proxy->setFilter(BaseVersionList::TypeRole, new ExactFilter("release"));
}

void ModFilterWidget::onVersionFilterChanged(int)
{
    auto versions = ui->versions->checkedItems();
    versions.sort();
    std::list<Version> current_list;

    for (const QString& version : versions)
        current_list.emplace_back(version);

    m_filter_changed = m_filter->versions.size() != current_list.size() ||
                       !std::equal(m_filter->versions.begin(), m_filter->versions.end(), current_list.begin(), current_list.end());
    m_filter->versions = current_list;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::onLoadersFilterChanged()
{
    ModPlatform::ModLoaderTypes loaders;
    if (ui->neoForge->isChecked())
        loaders |= ModPlatform::NeoForge;
    if (ui->forge->isChecked())
        loaders |= ModPlatform::Forge;
    if (ui->fabric->isChecked())
        loaders |= ModPlatform::Fabric;
    if (ui->quilt->isChecked())
        loaders |= ModPlatform::Quilt;
    m_filter_changed = loaders != m_filter->loaders;
    m_filter->loaders = loaders;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::onSideFilterChanged()
{
    QString side;

    if (ui->clientSide->isChecked() != ui->serverSide->isChecked()) {
        if (ui->clientSide->isChecked())
            side = "client";
        else
            side = "server";
    } else {
        // both are checked or none are checked; in either case no filtering will happen
        side = "";
    }

    m_filter_changed = side != m_filter->side;
    m_filter->side = side;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::onHideInstalledFilterChanged()
{
    auto hide = ui->hideInstalled->isChecked();
    m_filter_changed = hide != m_filter->hideInstalled;
    m_filter->hideInstalled = hide;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::onVersionFilterTextChanged(const QString& version)
{
    m_filter->versions.clear();
    if (ui->version->currentData(Qt::UserRole) != "all") {
        m_filter->versions.emplace_back(version);
    }
    m_filter_changed = true;
    emit filterChanged();
}

void ModFilterWidget::setCategories(const QList<ModPlatform::Category>& categories)
{
    m_categories = categories;

    delete ui->categoryGroup->layout();
    auto layout = new QVBoxLayout(ui->categoryGroup);

    for (const auto& category : categories) {
        auto name = category.name;
        name.replace("-", " ");
        name.replace("&", "&&");
        auto checkbox = new QCheckBox(name);
        auto font = checkbox->font();
        font.setCapitalization(QFont::Capitalize);
        checkbox->setFont(font);

        layout->addWidget(checkbox);

        const QString id = category.id;
        connect(checkbox, &QCheckBox::toggled, this, [this, id](bool checked) {
            if (checked)
                m_filter->categoryIds.append(id);
            else
                m_filter->categoryIds.removeOne(id);

            m_filter_changed = true;
            emit filterChanged();
        });
    }
}

#include "ModFilterWidget.moc"