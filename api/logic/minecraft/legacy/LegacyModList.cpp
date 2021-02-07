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

#include "LegacyModList.h"
#include <FileSystem.h>
#include <QString>
#include <QDebug>

LegacyModList::LegacyModList(const QString &dir, const QString &list_file)
    : m_dir(dir), m_list_file(list_file)
{
    FS::ensureFolderPathExists(m_dir.absolutePath());
    m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs | QDir::NoSymLinks);
    m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
}

    struct OrderItem
    {
        QString id;
        bool enabled = false;
    };
    typedef QList<OrderItem> OrderList;

static void internalSort(QList<LegacyModList::Mod> &what)
{
    auto predicate = [](const LegacyModList::Mod &left, const LegacyModList::Mod &right)
    {
        return left.fileName().localeAwareCompare(right.fileName()) < 0;
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
            orderedMods.append(info);
        }
    }
    // if there are any untracked files... append them sorted at the end
    if (folderContents.size())
    {
        for (auto entry : folderContents)
        {
            newMods.append(entry);
        }
        internalSort(newMods);
        orderedMods.append(newMods);
    }
    mods.swap(orderedMods);
    return true;
}
