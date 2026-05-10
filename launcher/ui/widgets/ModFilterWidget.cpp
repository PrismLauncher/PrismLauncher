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
#include "Json.h"
#include "Version.h"
#include "meta/Index.h"
#include "modplatform/ModIndex.h"
#include "ui/widgets/CheckComboBox.h"
#include "ui_ModFilterWidget.h"

#include "Application.h"
#include "minecraft/PackProfile.h"

ModFilterWidget* ModFilterWidget::create(MinecraftInstance* instance, bool extended)
{
    return new ModFilterWidget(instance, extended);
}

namespace {
class VersionBasicModel : public QIdentityProxyModel {
    Q_OBJECT

   public:
    explicit VersionBasicModel(QObject* parent = nullptr) : QIdentityProxyModel(parent) {}

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole) {
            return QIdentityProxyModel::data(index, BaseVersionList::VersionIdRole);
        }
        if (role == Qt::UserRole) {
            return QIdentityProxyModel::data(index, BaseVersionList::VersionIdRole);
        }
        return {};
    }
};

class AllVersionProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

   public:
    explicit AllVersionProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

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
}  // namespace

ModFilterWidget::ModFilterWidget(MinecraftInstance* instance, bool extended)
    : m_ui(new Ui::ModFilterWidget), m_instance(instance), m_filter(new Filter()), m_versionsProxy(new VersionProxyModel(this))
{
    m_ui->setupUi(this);

    m_versionsProxy->setFilter(BaseVersionList::TypeRole, Filters::equals("release"));

    QAbstractProxyModel* proxy = new VersionBasicModel(this);
    proxy->setSourceModel(m_versionsProxy);

    if (extended) {
        if (!m_instance) {
            m_ui->environmentGroup->hide();
        }
        m_ui->versions->setSourceModel(proxy);
        m_ui->versions->setSeparator(", ");
        m_ui->versions->setDefaultText(tr("All Versions"));
        m_ui->version->hide();
    } else {
        auto* allVersions = new AllVersionProxyModel(this);
        allVersions->setSourceModel(proxy);
        proxy = allVersions;
        m_ui->version->setModel(proxy);
        m_ui->versions->hide();
        m_ui->showAllVersions->hide();
        m_ui->environmentGroup->hide();
        m_ui->openSource->hide();
    }

    connect(m_ui->showAllVersions, &QCheckBox::stateChanged, this, &ModFilterWidget::onShowAllVersionsChanged);
    connect(m_ui->version, &QComboBox::currentTextChanged, this, &ModFilterWidget::onVersionFilterTextChanged);

    connect(m_ui->neoForge, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->forge, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->fabric, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->quilt, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->liteLoader, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->babric, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->btaBabric, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->legacyFabric, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->ornithe, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->rift, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onLoadersFilterChanged);

    connect(m_ui->showMoreButton, &QPushButton::clicked, this, &ModFilterWidget::onShowMoreClicked);

    if (!extended) {
        m_ui->showMoreButton->setVisible(false);
        m_ui->extendedModLoadersWidget->setVisible(false);
    }

    if (extended) {
        connect(m_ui->clientSide, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onSideFilterChanged);
        connect(m_ui->serverSide, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onSideFilterChanged);
    }

    connect(m_ui->hideInstalled, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onHideInstalledFilterChanged);
    connect(m_ui->openSource, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onOpenSourceFilterChanged);

    connect(m_ui->releaseCb, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(m_ui->betaCb, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(m_ui->alphaCb, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(m_ui->unknownCb, &QCheckBox::checkStateChanged, this, &ModFilterWidget::onReleaseFilterChanged);

    setHidden(true);
    loadVersionList();
    prepareBasicFilter();
}

auto ModFilterWidget::getFilter() -> std::shared_ptr<Filter>
{
    m_filterChanged = false;
    return m_filter;
}

ModFilterWidget::~ModFilterWidget()
{
    delete m_ui;
}

void ModFilterWidget::loadVersionList()
{
    m_versionList = APPLICATION->metadataIndex()->get("net.minecraft");
    if (!m_versionList->isLoaded()) {
        QEventLoop loadVersionListLoop;

        QTimer timeLimitForListLoad;
        timeLimitForListLoad.setTimerType(Qt::TimerType::CoarseTimer);
        timeLimitForListLoad.setSingleShot(true);
        timeLimitForListLoad.callOnTimeout(&loadVersionListLoop, &QEventLoop::quit);
        timeLimitForListLoad.start(4000);

        auto task = m_versionList->getLoadTask();

        connect(task.get(), &Task::failed, this, [this] {
            m_ui->versions->setEnabled(false);
            m_ui->showAllVersions->setEnabled(false);
        });
        connect(task.get(), &Task::finished, &loadVersionListLoop, &QEventLoop::quit);

        if (!task->isRunning()) {
            task->start();
        }

        loadVersionListLoop.exec();
        if (timeLimitForListLoad.isActive()) {
            timeLimitForListLoad.stop();
        }
    }
    m_versionsProxy->setSourceModel(m_versionList.get());
}

void ModFilterWidget::prepareBasicFilter()
{
    m_filter->openSource = false;
    if (m_instance) {
        m_filter->hideInstalled = false;
        m_filter->side = ModPlatform::SideType::NoSide;  // or "both"
        ModPlatform::ModLoaderTypes loaders;
        if (m_instance->settings()->get("OverrideModDownloadLoaders").toBool()) {
            for (const auto& loader : Json::toStringList(m_instance->settings()->get("ModDownloadLoaders").toString())) {
                loaders |= ModPlatform::getModLoaderFromString(loader);
            }
        } else {
            loaders = m_instance->getPackProfile()->getSupportedModLoaders().value_or(ModPlatform::ModLoaderTypes(0));
        }
        m_ui->neoForge->setChecked((loaders & ModPlatform::NeoForge) != 0U);
        m_ui->forge->setChecked((loaders & ModPlatform::Forge) != 0U);
        m_ui->fabric->setChecked((loaders & ModPlatform::Fabric) != 0U);
        m_ui->quilt->setChecked((loaders & ModPlatform::Quilt) != 0U);
        m_ui->liteLoader->setChecked((loaders & ModPlatform::LiteLoader) != 0U);
        m_ui->babric->setChecked((loaders & ModPlatform::Babric) != 0U);
        m_ui->btaBabric->setChecked((loaders & ModPlatform::BTA) != 0U);
        m_ui->legacyFabric->setChecked((loaders & ModPlatform::LegacyFabric) != 0U);
        m_ui->ornithe->setChecked((loaders & ModPlatform::Ornithe) != 0U);
        m_ui->rift->setChecked((loaders & ModPlatform::Rift) != 0U);
        m_filter->loaders = loaders;
        auto def = m_instance->getPackProfile()->getComponentVersion("net.minecraft");
        m_filter->versions.emplace_back(def);
        m_ui->versions->setCheckedItems({ def });
        m_ui->version->setCurrentIndex(m_ui->version->findText(def));
    } else {
        m_ui->hideInstalled->hide();
    }
}

void ModFilterWidget::onShowAllVersionsChanged()
{
    if (m_ui->showAllVersions->isChecked()) {
        m_versionsProxy->clearFilters();
    } else {
        m_versionsProxy->setFilter(BaseVersionList::TypeRole, Filters::equals("release"));
    }
}

void ModFilterWidget::onVersionFilterChanged(int /*unused*/)
{
    auto versions = m_ui->versions->checkedItems();
    versions.sort();
    std::vector<Version> currentList;

    for (const QString& version : versions) {
        currentList.emplace_back(version);
    }

    m_filterChanged = m_filter->versions.size() != currentList.size() ||
                      !std::equal(m_filter->versions.begin(), m_filter->versions.end(), currentList.begin(), currentList.end());
    m_filter->versions = currentList;
    if (m_filterChanged) {
        emit filterChanged();
    }
}

void ModFilterWidget::onLoadersFilterChanged()
{
    ModPlatform::ModLoaderTypes loaders;
    if (m_ui->neoForge->isChecked()) {
        loaders |= ModPlatform::NeoForge;
    }
    if (m_ui->forge->isChecked()) {
        loaders |= ModPlatform::Forge;
    }
    if (m_ui->fabric->isChecked()) {
        loaders |= ModPlatform::Fabric;
    }
    if (m_ui->quilt->isChecked()) {
        loaders |= ModPlatform::Quilt;
    }
    if (m_ui->liteLoader->isChecked()) {
        loaders |= ModPlatform::LiteLoader;
    }
    if (m_ui->babric->isChecked()) {
        loaders |= ModPlatform::Babric;
    }
    if (m_ui->btaBabric->isChecked()) {
        loaders |= ModPlatform::BTA;
    }
    if (m_ui->legacyFabric->isChecked()) {
        loaders |= ModPlatform::LegacyFabric;
    }
    if (m_ui->ornithe->isChecked()) {
        loaders |= ModPlatform::Ornithe;
    }
    if (m_ui->rift->isChecked()) {
        loaders |= ModPlatform::Rift;
    }
    m_filterChanged = loaders != m_filter->loaders;
    m_filter->loaders = loaders;
    if (m_filterChanged) {
        emit filterChanged();
    }
}

void ModFilterWidget::onSideFilterChanged()
{
    ModPlatform::SideType side;

    if (m_ui->clientSide->isChecked() && !m_ui->serverSide->isChecked()) {
        side = ModPlatform::SideType::ClientSide;
    } else if (!m_ui->clientSide->isChecked() && m_ui->serverSide->isChecked()) {
        side = ModPlatform::SideType::ServerSide;
    } else if (m_ui->clientSide->isChecked() && m_ui->serverSide->isChecked()) {
        side = ModPlatform::SideType::UniversalSide;
    } else {
        side = ModPlatform::SideType::NoSide;
    }

    m_filterChanged = side != m_filter->side;
    m_filter->side = side;
    if (m_filterChanged) {
        emit filterChanged();
    }
}

void ModFilterWidget::onHideInstalledFilterChanged()
{
    auto hide = m_ui->hideInstalled->isChecked();
    m_filterChanged = hide != m_filter->hideInstalled;
    m_filter->hideInstalled = hide;
    if (m_filterChanged) {
        emit filterChanged();
    }
}

void ModFilterWidget::onVersionFilterTextChanged(const QString& version)
{
    m_filter->versions.clear();
    if (m_ui->version->currentData(Qt::UserRole) != "all") {
        m_filter->versions.emplace_back(version);
    }
    m_filterChanged = true;
    emit filterChanged();
}

void ModFilterWidget::setCategories(const QList<ModPlatform::Category>& categories)
{
    m_categories = categories;

    delete m_ui->categoryGroup->layout();
    auto* layout = new QVBoxLayout(m_ui->categoryGroup);

    for (const auto& category : categories) {
        auto name = category.name;
        name.replace("-", " ");
        name.replace("&", "&&");
        auto* checkbox = new QCheckBox(name);
        auto font = checkbox->font();
        font.setCapitalization(QFont::Capitalize);
        checkbox->setFont(font);

        layout->addWidget(checkbox);

        const QString id = category.id;
        connect(checkbox, &QCheckBox::toggled, this, [this, id](bool checked) {
            if (checked) {
                m_filter->categoryIds.append(id);
            } else {
                m_filter->categoryIds.removeOne(id);
            }

            m_filterChanged = true;
            emit filterChanged();
        });
    }
}

void ModFilterWidget::onOpenSourceFilterChanged()
{
    auto open = m_ui->openSource->isChecked();
    m_filterChanged = open != m_filter->openSource;
    m_filter->openSource = open;
    if (m_filterChanged) {
        emit filterChanged();
    }
}

void ModFilterWidget::onReleaseFilterChanged()
{
    std::vector<ModPlatform::IndexedVersionType> releases;
    if (m_ui->releaseCb->isChecked()) {
        releases.emplace_back(ModPlatform::IndexedVersionType::Release);
    }
    if (m_ui->betaCb->isChecked()) {
        releases.emplace_back(ModPlatform::IndexedVersionType::Beta);
    }
    if (m_ui->alphaCb->isChecked()) {
        releases.emplace_back(ModPlatform::IndexedVersionType::Alpha);
    }
    if (m_ui->unknownCb->isChecked()) {
        releases.emplace_back(ModPlatform::IndexedVersionType::Unknown);
    }
    m_filterChanged = releases != m_filter->releases;
    m_filter->releases = releases;
    if (m_filterChanged) {
        emit filterChanged();
    }
}

void ModFilterWidget::onShowMoreClicked()
{
    m_ui->extendedModLoadersWidget->setVisible(true);
    m_ui->showMoreButton->setVisible(false);
}

#include "ModFilterWidget.moc"
