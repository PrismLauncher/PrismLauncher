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
#include "settings/Setting.h"
#include "ui/widgets/CheckComboBox.h"
#include "ui_ModFilterWidget.h"

#include "Application.h"
#include "minecraft/PackProfile.h"

unique_qobject_ptr<ModFilterWidget> ModFilterWidget::create(MinecraftInstance* instance,
                                                            ModPlatform::ResourceProvider provider,
                                                            QWidget* parent)
{
    return unique_qobject_ptr<ModFilterWidget>(new ModFilterWidget(instance, provider, parent));
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

ModFilterWidget::ModFilterWidget(MinecraftInstance* instance, ModPlatform::ResourceProvider provider, QWidget* parent)
    : QTabWidget(parent), m_ui(new Ui::ModFilterWidget), m_instance(instance), m_filter(new Filter()), m_provider(provider)
{
    m_ui->setupUi(this);

    m_versions_proxy = new VersionProxyModel(this);
    m_versions_proxy->setFilter(BaseVersionList::TypeRole, new ExactFilter("release"));

    QAbstractProxyModel* proxy = new VersionBasicModel(this);
    proxy->setSourceModel(m_versions_proxy);

    if (m_provider == ModPlatform::ResourceProvider::MODRINTH) {
        if (!m_instance) {
            m_ui->environmentGroup->hide();
        }
        m_ui->versions->setSourceModel(proxy);
        m_ui->versions->setSeparator(", ");
        m_ui->versions->setDefaultText(tr("All Versions"));
        m_ui->version->hide();
    } else {
        auto allVersions = new AllVersionProxyModel(this);
        allVersions->setSourceModel(proxy);
        proxy = allVersions;
        m_ui->version->setModel(proxy);
        m_ui->versions->hide();
        m_ui->showAllVersions->hide();
        m_ui->environmentGroup->hide();
        m_ui->openSource->hide();
    }

    if (!m_instance) {
        m_ui->hideInstalled->hide();
        m_ui->resetButton->hide();
        m_ui->saveFiltersCb->hide();
    } else {
        auto setting = m_instance->settings()->getOrRegisterSetting(getSettingId("filtersSaved"));
        m_ui->saveFiltersCb->setChecked(setting->get().toBool());
        connect(setting.get(), &Setting::SettingChanged, this,
                [this](const Setting&, const QVariant& value) { m_ui->saveFiltersCb->setChecked(value.toBool()); });
        connect(m_ui->saveFiltersCb, &QCheckBox::stateChanged, this, [this] {
            saveSetting("filtersSaved", m_ui->saveFiltersCb->isChecked());
            if (m_ui->saveFiltersCb->isChecked()) {
                saveFilters();
            }
        });
    }

    m_ui->versions->setStyleSheet("combobox-popup: 0;");
    m_ui->version->setStyleSheet("combobox-popup: 0;");

    loadVersionList();
    loadFilters();

    connect(m_ui->showAllVersions, &QCheckBox::stateChanged, this, &ModFilterWidget::onShowAllVersionsChanged);
    connect(m_ui->versions, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModFilterWidget::onVersionFilterChanged);
    connect(m_ui->versions, &CheckComboBox::checkedItemsChanged, this, [this] { onVersionFilterChanged(0); });
    connect(m_ui->version, &QComboBox::currentTextChanged, this, &ModFilterWidget::onVersionFilterTextChanged);

    connect(m_ui->neoForge, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->forge, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->fabric, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
    connect(m_ui->quilt, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);

    if (m_provider == ModPlatform::ResourceProvider::MODRINTH) {
        connect(m_ui->liteLoader, &QCheckBox::stateChanged, this, &ModFilterWidget::onLoadersFilterChanged);
        connect(m_ui->clientSide, &QCheckBox::stateChanged, this, &ModFilterWidget::onSideFilterChanged);
        connect(m_ui->serverSide, &QCheckBox::stateChanged, this, &ModFilterWidget::onSideFilterChanged);
    } else {
        m_ui->liteLoader->setVisible(false);
    }

    connect(m_ui->hideInstalled, &QCheckBox::stateChanged, this, &ModFilterWidget::onHideInstalledFilterChanged);
    connect(m_ui->openSource, &QCheckBox::stateChanged, this, &ModFilterWidget::onOpenSourceFilterChanged);

    connect(m_ui->releaseCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(m_ui->betaCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(m_ui->alphaCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onReleaseFilterChanged);
    connect(m_ui->unknownCb, &QCheckBox::stateChanged, this, &ModFilterWidget::onReleaseFilterChanged);

    connect(this, &ModFilterWidget::filterChanged, this, &ModFilterWidget::saveFilters);
    connect(m_ui->resetButton, &QPushButton::clicked, this, &ModFilterWidget::clearFilter);
    setHidden(true);
}

auto ModFilterWidget::getFilter() -> std::shared_ptr<Filter>
{
    m_filter_changed = false;
    return m_filter;
}

ModFilterWidget::~ModFilterWidget()
{
    delete m_ui;
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
            m_ui->versions->setEnabled(false);
            m_ui->showAllVersions->setEnabled(false);
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

void ModFilterWidget::clearFilter()
{
    m_filter->openSource = false;
    if (m_instance) {
        m_filter->hideInstalled = false;
        m_filter->side = "";  // or "both"
        auto loaders = m_instance->getPackProfile()->getSupportedModLoaders().value();
        m_ui->neoForge->setChecked(loaders & ModPlatform::NeoForge);
        m_ui->forge->setChecked(loaders & ModPlatform::Forge);
        m_ui->fabric->setChecked(loaders & ModPlatform::Fabric);
        m_ui->quilt->setChecked(loaders & ModPlatform::Quilt);
        m_ui->liteLoader->setChecked(loaders & ModPlatform::LiteLoader);
        m_filter->loaders = loaders;
        auto def = m_instance->getPackProfile()->getComponentVersion("net.minecraft");
        m_filter->versions.emplace_front(def);
        m_ui->versions->setCheckedItems({ def });
        m_ui->version->setCurrentIndex(m_ui->version->findText(def));
        m_ui->releaseCb->setChecked(false);
        m_ui->betaCb->setChecked(false);
        m_ui->alphaCb->setChecked(false);
        m_ui->unknownCb->setChecked(false);

        m_ui->hideInstalled->setChecked(false);
        m_ui->openSource->setChecked(false);
        m_ui->clientSide->setChecked(false);
        m_ui->serverSide->setChecked(false);

        for (const auto& key : m_categoriesCB.keys()) {
            m_categoriesCB.value(key)->setChecked(false);
        }
    }
}

void ModFilterWidget::onShowAllVersionsChanged()
{
    if (m_ui->showAllVersions->isChecked())
        m_versions_proxy->clearFilters();
    else
        m_versions_proxy->setFilter(BaseVersionList::TypeRole, new ExactFilter("release"));
}

void ModFilterWidget::onVersionFilterChanged(int)
{
    auto versions = m_ui->versions->checkedItems();
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
    if (m_ui->neoForge->isChecked())
        loaders |= ModPlatform::NeoForge;
    if (m_ui->forge->isChecked())
        loaders |= ModPlatform::Forge;
    if (m_ui->fabric->isChecked())
        loaders |= ModPlatform::Fabric;
    if (m_ui->quilt->isChecked())
        loaders |= ModPlatform::Quilt;
    if (m_ui->liteLoader->isChecked())
        loaders |= ModPlatform::LiteLoader;
    m_filter_changed = loaders != m_filter->loaders;
    m_filter->loaders = loaders;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::onSideFilterChanged()
{
    QString side;

    if (m_ui->clientSide->isChecked() && !m_ui->serverSide->isChecked()) {
        side = "client";
    } else if (!m_ui->clientSide->isChecked() && m_ui->serverSide->isChecked()) {
        side = "server";
    } else if (m_ui->clientSide->isChecked() && m_ui->serverSide->isChecked()) {
        side = "both";
    } else {
        side = "";
    }

    m_filter_changed = side != m_filter->side;
    m_filter->side = side;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::onHideInstalledFilterChanged()
{
    auto hide = m_ui->hideInstalled->isChecked();
    m_filter_changed = hide != m_filter->hideInstalled;
    m_filter->hideInstalled = hide;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::onVersionFilterTextChanged(const QString& version)
{
    m_filter->versions.clear();
    if (m_ui->version->currentData(Qt::UserRole) != "all") {
        m_filter->versions.emplace_back(version);
    }
    m_filter_changed = true;
    emit filterChanged();
}

void ModFilterWidget::setCategories(const QList<ModPlatform::Category>& categories)
{
    m_categories = categories;
    m_categoriesCB.clear();

    delete m_ui->categoryGroup->layout();
    auto layout = new QVBoxLayout(m_ui->categoryGroup);

    for (const auto& category : categories) {
        auto name = category.name;
        name.replace("-", " ");
        name.replace("&", "&&");
        auto checkbox = new QCheckBox(name);
        auto font = checkbox->font();
        font.setCapitalization(QFont::Capitalize);
        checkbox->setFont(font);
        const QString id = category.id;
        checkbox->setChecked(m_filter->categoryIds.contains(id));
        m_categoriesCB[id] = checkbox;
        layout->addWidget(checkbox);

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

void ModFilterWidget::onOpenSourceFilterChanged()
{
    auto open = m_ui->openSource->isChecked();
    m_filter_changed = open != m_filter->openSource;
    m_filter->openSource = open;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::onReleaseFilterChanged()
{
    std::list<ModPlatform::IndexedVersionType> releases;
    if (m_ui->releaseCb->isChecked())
        releases.push_back(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Release));
    if (m_ui->betaCb->isChecked())
        releases.push_back(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Beta));
    if (m_ui->alphaCb->isChecked())
        releases.push_back(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Alpha));
    if (m_ui->unknownCb->isChecked())
        releases.push_back(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Unknown));
    m_filter_changed = releases != m_filter->releases;
    m_filter->releases = releases;
    if (m_filter_changed)
        emit filterChanged();
}

void ModFilterWidget::saveSetting(QString id, QVariant value)
{
    auto const setting_name = getSettingId(id);
    auto setting = m_instance->settings()->getOrRegisterSetting(setting_name);

    setting->set(value);
}

QVariant ModFilterWidget::getSetting(QString id)
{
    auto const setting_name = getSettingId(id);
    auto setting = m_instance->settings()->getOrRegisterSetting(setting_name);

    return setting->get();
}

void ModFilterWidget::saveFilters()
{
    if (!m_instance) {
        return;
    }
    QStringList versionStrings;
    for (const auto& v : m_filter->versions)
        versionStrings.append(v.toString());

    QStringList releaseStrings;
    for (const auto& r : m_filter->releases)
        releaseStrings.append(r.toString());

    QStringList loaders;
    for (auto loader : ModPlatform::modLoaderTypesToList(m_filter->loaders)) {
        loaders.push_back(getModLoaderAsString(loader));
    }
    saveSetting("versions", versionStrings);
    saveSetting("releases", releaseStrings);
    saveSetting("loaders", loaders);
    saveSetting("side", m_filter->side);
    saveSetting("hideInstalled", m_filter->hideInstalled);
    saveSetting("categoryIds", m_filter->categoryIds);
    saveSetting("openSource", m_filter->openSource);
}

void ModFilterWidget::loadFilters()
{
    if (!m_instance || !getSetting("filtersSaved").toBool()) {
        clearFilter();
        return;
    }
    QStringList versionStrings = getSetting("versions").toStringList();
    for (const auto& v : versionStrings)
        m_filter->versions.push_back(Version(v));

    QStringList releaseStrings = getSetting("releases").toStringList();
    for (const auto& r : releaseStrings)
        m_filter->releases.push_back(ModPlatform::IndexedVersionType(r));

    QStringList loaders = getSetting("loaders").toStringList();
    for (auto&& loader : loaders) {
        m_filter->loaders |= ModPlatform::getModLoaderFromString(loader);
    }

    m_filter->side = getSetting("side").toString();
    m_filter->hideInstalled = getSetting("hideInstalled").toBool();
    m_filter->categoryIds = getSetting("categoryIds").toStringList();
    m_filter->openSource = getSetting("openSource").toBool();

    m_ui->hideInstalled->setChecked(m_filter->hideInstalled);
    m_ui->openSource->setChecked(m_filter->openSource);

    if (m_filter->side == "client") {
        m_ui->clientSide->setChecked(true);
    } else if (m_filter->side == "server") {
        m_ui->serverSide->setChecked(true);
    } else if (m_filter->side == "both") {
        m_ui->clientSide->setChecked(true);
        m_ui->serverSide->setChecked(true);
    }
    m_ui->neoForge->setChecked(m_filter->loaders & ModPlatform::NeoForge);
    m_ui->forge->setChecked(m_filter->loaders & ModPlatform::Forge);
    m_ui->fabric->setChecked(m_filter->loaders & ModPlatform::Fabric);
    m_ui->quilt->setChecked(m_filter->loaders & ModPlatform::Quilt);

    m_ui->versions->setCheckedItems(versionStrings);
    m_ui->version->setCurrentIndex(m_ui->version->findText(m_filter->versions.front().toString()));

    m_ui->releaseCb->setChecked(
        releaseStrings.contains(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Release).toString()));
    m_ui->betaCb->setChecked(
        releaseStrings.contains(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Beta).toString()));
    m_ui->alphaCb->setChecked(
        releaseStrings.contains(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Alpha).toString()));
    m_ui->unknownCb->setChecked(
        releaseStrings.contains(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::VersionType::Unknown).toString()));
}

QString ModFilterWidget::getSettingId(QString id)
{
    return QString("ModFilter/%1/%2").arg(ModPlatform::ProviderCapabilities::readableName(m_provider), id);
}
#include "ModFilterWidget.moc"
