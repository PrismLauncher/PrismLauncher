/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "InstanceProxyModel.h"

#include <BaseInstance.h>
#include <icons/IconList.h>
#include "Application.h"
#include "InstanceView.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/ModIndex.h"

#include <QDebug>
#include <utility>

InstanceProxyModel::InstanceProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    m_naturalSort.setNumericMode(true);
    m_naturalSort.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    // FIXME: use loaded translation as source of locale instead, hook this up to translation changes
    m_naturalSort.setLocale(QLocale::system());
}
void InstanceProxyModel::sortBy(QStringList mcVersions, ModPlatform::ModLoaderTypes loader)
{
    m_mcVersions = std::move(mcVersions);
    m_loader = loader;
}

QVariant InstanceProxyModel::data(const QModelIndex& index, int role) const
{
    QVariant data = QSortFilterProxyModel::data(index, role);
    if (role == Qt::DecorationRole) {
        return QVariant(APPLICATION->icons()->getIcon(data.toString()));
    }
    return data;
}

bool InstanceProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const QString leftCategory = left.data(InstanceViewRoles::GroupRole).toString();
    const QString rightCategory = right.data(InstanceViewRoles::GroupRole).toString();
    if (leftCategory == rightCategory) {
        return subSortLessThan(left, right);
    }  // FIXME: real group sorting happens in InstanceView::updateGeometries(), see LocaleString
    auto result = leftCategory.localeAwareCompare(rightCategory);
    if (result == 0) {
        return subSortLessThan(left, right);
    }
    return result < 0;
}

bool InstanceProxyModel::subSortLessThan(const QModelIndex& left, const QModelIndex& right) const
{
    auto* pdataLeft = static_cast<BaseInstance*>(left.internalPointer());
    auto* pdataRight = static_cast<BaseInstance*>(right.internalPointer());
    QString sortMode = APPLICATION->settings()->get("InstSortMode").toString();
    if (sortMode == "LastLaunch") {
        return pdataLeft->lastLaunch() > pdataRight->lastLaunch();
    }
    if (sortMode == "Playtime") {
        if (pdataLeft->totalTimePlayed() == pdataRight->totalTimePlayed()) {
            return m_naturalSort.compare(pdataLeft->name(), pdataRight->name()) < 0;
        }
        return pdataLeft->totalTimePlayed() > pdataRight->totalTimePlayed();
    }
    return m_naturalSort.compare(pdataLeft->name(), pdataRight->name()) < 0;
}

bool InstanceProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_mcVersions.isEmpty() && m_loader == ModPlatform::ModLoaderType::None) {
        return true;
    }
    auto data = sourceModel()->index(sourceRow, 0, sourceParent);
    auto* inst = static_cast<MinecraftInstance*>(data.internalPointer());
    auto* profile = inst->getPackProfile();
    if ((profile == nullptr) || profile->rowCount() == 0) {
        return true;
    }
    const auto mcVersion = profile->getComponentVersion("net.minecraft");
    auto loader = profile->getSupportedModLoaders().value_or(ModPlatform::ModLoaderTypes(0));
    if (!m_mcVersions.isEmpty() && !m_mcVersions.contains(mcVersion)) {
        return false;
    }
    if (m_loader != ModPlatform::ModLoaderType::None && !loader.testAnyFlags(m_loader)) {
        return false;
    }
    return true;
}
