/* Copyright 2013-2018 MultiMC Contributors
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

#include <QDir>
#include <QSet>
#include <QFile>
#include <QThread>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QDebug>

#include "InstanceList.h"
#include "BaseInstance.h"

#include "FolderInstanceProvider.h"
#include "FileSystem.h"

InstanceList::InstanceList(QObject *parent)
    : QAbstractListModel(parent)
{
    resumeWatch();
}

InstanceList::~InstanceList()
{
}

int InstanceList::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_instances.count();
}

QModelIndex InstanceList::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row < 0 || row >= m_instances.size())
        return QModelIndex();
    return createIndex(row, column, (void *)m_instances.at(row).get());
}

QVariant InstanceList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }
    BaseInstance *pdata = static_cast<BaseInstance *>(index.internalPointer());
    switch (role)
    {
    case InstancePointerRole:
    {
        QVariant v = qVariantFromValue((void *)pdata);
        return v;
    }
    case InstanceIDRole:
    {
        return pdata->id();
    }
    case Qt::DisplayRole:
    {
        return pdata->name();
    }
    case Qt::ToolTipRole:
    {
        return pdata->instanceRoot();
    }
    case Qt::DecorationRole:
    {
        return pdata->iconKey();
    }
    // HACK: see GroupView.h in gui!
    case GroupRole:
    {
        return pdata->group();
    }
    default:
        break;
    }
    return QVariant();
}

Qt::ItemFlags InstanceList::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f;
    if (index.isValid())
    {
        f |= (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
    return f;
}

QStringList InstanceList::getGroups()
{
    return m_groups.toList();
}

void InstanceList::deleteGroup(const QString& name)
{
    for(auto & instance: m_instances)
    {
        auto instGroupName = instance->group();
        if(instGroupName == name)
        {
            instance->setGroupPost(QString());
        }
    }
}

void InstanceList::deleteInstance(const InstanceId& id)
{
    auto inst = getInstanceById(id);
    if(!inst)
    {
        qDebug() << "Cannot delete instance" << id << " No such instance is present.";
        return;
    }

    qDebug() << "Will delete instance" << id;
    if(!FS::deletePath(inst->instanceRoot()))
    {
        qWarning() << "Deletion of instance" << id << "has not been completely successful ...";
        return;
    }

    qDebug() << "Instance" << id << "has been deleted by MultiMC.";
}

static QMap<InstanceId, InstanceLocator> getIdMapping(const QList<InstancePtr> &list)
{
    QMap<InstanceId, InstanceLocator> out;
    int i = 0;
    for(auto & item: list)
    {
        auto id = item->id();
        if(out.contains(id))
        {
            qWarning() << "Duplicate ID" << id << "in instance list";
        }
        out[id] = std::make_pair(item, i);
        i++;
    }
    return out;
}

InstanceList::InstListError InstanceList::loadList()
{
    auto existingIds = getIdMapping(m_instances);

    QList<InstancePtr> newList;

    for(auto & id: m_provider->discoverInstances())
    {
        if(existingIds.contains(id))
        {
            auto instPair = existingIds[id];
            existingIds.remove(id);
            qDebug() << "Should keep and soft-reload" << id;
        }
        else
        {
            InstancePtr instPtr = m_provider->loadInstance(id);
            if(instPtr)
            {
                newList.append(instPtr);
            }
        }
    }

    // TODO: looks like a general algorithm with a few specifics inserted. Do something about it.
    if(!existingIds.isEmpty())
    {
        // get the list of removed instances and sort it by their original index, from last to first
        auto deadList = existingIds.values();
        auto orderSortPredicate = [](const InstanceLocator & a, const InstanceLocator & b) -> bool
        {
            return a.second > b.second;
        };
        std::sort(deadList.begin(), deadList.end(), orderSortPredicate);
        // remove the contiguous ranges of rows
        int front_bookmark = -1;
        int back_bookmark = -1;
        int currentItem = -1;
        auto removeNow = [&]()
        {
            beginRemoveRows(QModelIndex(), front_bookmark, back_bookmark);
            m_instances.erase(m_instances.begin() + front_bookmark, m_instances.begin() + back_bookmark + 1);
            endRemoveRows();
            front_bookmark = -1;
            back_bookmark = currentItem;
        };
        for(auto & removedItem: deadList)
        {
            auto instPtr = removedItem.first;
            instPtr->invalidate();
            currentItem = removedItem.second;
            if(back_bookmark == -1)
            {
                // no bookmark yet
                back_bookmark = currentItem;
            }
            else if(currentItem == front_bookmark - 1)
            {
                // part of contiguous sequence, continue
            }
            else
            {
                // seam between previous and current item
                removeNow();
            }
            front_bookmark = currentItem;
        }
        if(back_bookmark != -1)
        {
            removeNow();
        }
    }
    if(newList.size())
    {
        add(newList);
    }
    m_dirty = false;
    return NoError;
}

void InstanceList::saveNow()
{
    for(auto & item: m_instances)
    {
        item->saveNow();
    }
}

void InstanceList::add(const QList<InstancePtr> &t)
{
    beginInsertRows(QModelIndex(), m_instances.count(), m_instances.count() + t.size() - 1);
    m_instances.append(t);
    for(auto & ptr : t)
    {
        connect(ptr.get(), &BaseInstance::propertiesChanged, this, &InstanceList::propertiesChanged);
    }
    endInsertRows();
}

void InstanceList::resumeWatch()
{
    if(m_watchLevel > 0)
    {
        qWarning() << "Bad suspend level resume in instance list";
        return;
    }
    m_watchLevel++;
    if(m_watchLevel > 0 && m_dirty)
    {
        loadList();
    }
}

void InstanceList::suspendWatch()
{
    m_watchLevel --;
}

void InstanceList::providerUpdated()
{
    auto provider = dynamic_cast<FolderInstanceProvider *>(QObject::sender());
    if(!provider)
    {
        qWarning() << "InstanceList::providerUpdated triggered by a non-provider";
        return;
    }
    m_dirty = true;
    if(m_watchLevel == 1)
    {
        loadList();
    }
}

void InstanceList::groupsPublished(QSet<QString> newGroups)
{
    m_groups.unite(newGroups);
}

void InstanceList::addInstanceProvider(FolderInstanceProvider* provider)
{
    connect(provider, &FolderInstanceProvider::instancesChanged, this, &InstanceList::providerUpdated);
    connect(provider, &FolderInstanceProvider::groupsChanged, this, &InstanceList::groupsPublished);
    m_provider = provider;
}

InstancePtr InstanceList::getInstanceById(QString instId) const
{
    if(instId.isEmpty())
        return InstancePtr();
    for(auto & inst: m_instances)
    {
        if (inst->id() == instId)
        {
            return inst;
        }
    }
    return InstancePtr();
}

QModelIndex InstanceList::getInstanceIndexById(const QString &id) const
{
    return index(getInstIndex(getInstanceById(id).get()));
}

int InstanceList::getInstIndex(BaseInstance *inst) const
{
    int count = m_instances.count();
    for (int i = 0; i < count; i++)
    {
        if (inst == m_instances[i].get())
        {
            return i;
        }
    }
    return -1;
}

void InstanceList::propertiesChanged(BaseInstance *inst)
{
    int i = getInstIndex(inst);
    if (i != -1)
    {
        emit dataChanged(index(i), index(i));
    }
}
