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

#include "LegacyModList.h"
#include <FileSystem.h>
#include <QString>
#include <QDebug>

LegacyModList::LegacyModList(const QString &dir, const QString &list_file)
    : m_dir(dir), m_list_file(list_file)
{
    FS::ensureFolderPathExists(m_dir.absolutePath());
    m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs |
                    QDir::NoSymLinks);
    m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
}

    struct OrderItem
    {
        QString id;
        bool enabled = false;
    };
    typedef QList<OrderItem> OrderList;

static void internalSort(QList<Mod> &what)
{
    auto predicate = [](const Mod &left, const Mod &right)
    {
        if (left.name() == right.name())
        {
            return left.mmc_id().localeAwareCompare(right.mmc_id()) < 0;
        }
        return left.name().localeAwareCompare(right.name()) < 0;
    };
    std::sort(what.begin(), what.end(), predicate);
}

static OrderList readListFile(const QString &m_list_file)
{
    OrderList itemList;
    if (m_list_file.isNull() || m_list_file.isEmpty())
        return itemList;

    QFile textFile(m_list_file);
    if (!textFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return OrderList();

    QTextStream textStream;
    textStream.setAutoDetectUnicode(true);
    textStream.setDevice(&textFile);
    while (true)
    {
        QString line = textStream.readLine();
        if (line.isNull() || line.isEmpty())
            break;
        else
        {
            OrderItem it;
            it.enabled = !line.endsWith(".disabled");
            if (!it.enabled)
            {
                line.chop(9);
            }
            it.id = line;
            itemList.append(it);
        }
    }
    textFile.close();
    return itemList;
}

bool LegacyModList::update()
{
    if (!m_dir.exists() || !m_dir.isReadable())
        return false;

    QList<Mod> orderedMods;
    QList<Mod> newMods;
    m_dir.refresh();
    auto folderContents = m_dir.entryInfoList();
    bool orderOrStateChanged = false;

    // first, process the ordered items (if any)
    OrderList listOrder = readListFile(m_list_file);
    for (auto item : listOrder)
    {
        QFileInfo infoEnabled(m_dir.filePath(item.id));
        QFileInfo infoDisabled(m_dir.filePath(item.id + ".disabled"));
        int idxEnabled = folderContents.indexOf(infoEnabled);
        int idxDisabled = folderContents.indexOf(infoDisabled);
        bool isEnabled;
        // if both enabled and disabled versions are present, it's a special case...
        if (idxEnabled >= 0 && idxDisabled >= 0)
        {
            // we only process the one we actually have in the order file.
            // and exactly as we have it.
            // THIS IS A CORNER CASE
            isEnabled = item.enabled;
        }
        else
        {
            // only one is present.
            // we pick the one that we found.
            // we assume the mod was enabled/disabled by external means
            isEnabled = idxEnabled >= 0;
        }
        int idx = isEnabled ? idxEnabled : idxDisabled;
        QFileInfo &info = isEnabled ? infoEnabled : infoDisabled;
        // if the file from the index file exists
        if (idx != -1)
        {
            // remove from the actual folder contents list
            folderContents.takeAt(idx);
            // append the new mod
            orderedMods.append(Mod(info));
            if (isEnabled != item.enabled)
                orderOrStateChanged = true;
        }
        else
        {
            orderOrStateChanged = true;
        }
    }
    // if there are any untracked files...
    if (folderContents.size())
    {
        // the order surely changed!
        for (auto entry : folderContents)
        {
            newMods.append(Mod(entry));
        }
        internalSort(newMods);
        orderedMods.append(newMods);
        orderOrStateChanged = true;
    }
    // otherwise, if we were already tracking some mods
    else if (mods.size())
    {
        // if the number doesn't match, order changed.
        if (mods.size() != orderedMods.size())
            orderOrStateChanged = true;
        // if it does match, compare the mods themselves
        else
            for (int i = 0; i < mods.size(); i++)
            {
                if (!mods[i].strongCompare(orderedMods[i]))
                {
                    orderOrStateChanged = true;
                    break;
                }
            }
    }
    mods.swap(orderedMods);
    if (orderOrStateChanged && !m_list_file.isEmpty())
    {
        qDebug() << "Mod list " << m_list_file << " changed!";
    }
    return true;
}
