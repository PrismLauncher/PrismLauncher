// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "IconList.h"
#include <FileSystem.h>
#include <QDebug>
#include <QEventLoop>
#include <QFileSystemWatcher>
#include <QMap>
#include <QMimeData>
#include <QSet>
#include <QUrl>
#include "icons/IconUtils.h"

#define MAX_SIZE 1024

IconList::IconList(const QStringList& builtinPaths, const QString& path, QObject* parent) : QAbstractListModel(parent)
{
    QSet<QString> builtinNames;

    // add builtin icons
    for (const auto& builtinPath : builtinPaths) {
        QDir instance_icons(builtinPath);
        auto file_info_list = instance_icons.entryInfoList(QDir::Files, QDir::Name);
        for (const auto& file_info : file_info_list) {
            builtinNames.insert(file_info.baseName());
        }
    }
    for (const auto& builtinName : builtinNames) {
        addThemeIcon(builtinName);
    }

    m_watcher.reset(new QFileSystemWatcher());
    is_watching = false;
    connect(m_watcher.get(), &QFileSystemWatcher::directoryChanged, this, &IconList::directoryChanged);
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this, &IconList::fileChanged);

    directoryChanged(path);

    // Forces the UI to update, so that lengthy icon names are shown properly from the start
    emit iconUpdated({});
}

void IconList::sortIconList()
{
    qDebug() << "Sorting icon list...";
    std::sort(icons.begin(), icons.end(), [](const MMCIcon& a, const MMCIcon& b) {
        bool aIsSubdir = a.m_key.contains(QDir::separator());
        bool bIsSubdir = b.m_key.contains(QDir::separator());
        if (aIsSubdir != bIsSubdir) {
            return !aIsSubdir;  // root-level icons come first
        }
        return a.m_key.localeAwareCompare(b.m_key) < 0;
    });
    reindex();
}

// Helper function to add directories recursively
bool IconList::addPathRecursively(const QString& path)
{
    QDir dir(path);
    if (!dir.exists())
        return false;

    // Add the directory itself
    bool watching = m_watcher->addPath(path);

    // Add all subdirectories
    QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        if (addPathRecursively(entry.absoluteFilePath())) {
            watching = true;
        }
    }
    return watching;
}

void IconList::removePathRecursively(const QString& path)
{
    QFileInfo file_info(path);
    if (file_info.isFile()) {
        // Remove the icon belonging to the file
        QString key = m_dir.relativeFilePath(file_info.absoluteFilePath());
        int idx = getIconIndex(key);
        if (idx == -1)
            return;

    } else if (file_info.isDir()) {
        // Remove the directory itself
        m_watcher->removePath(path);

        const QDir dir(path);
        // Remove all files within the directory
        for (const QFileInfo& file : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            removePathRecursively(file.absoluteFilePath());
        }
    }
}

QStringList IconList::getIconFilePaths() const
{
    QStringList iconFiles{};
    QStringList directories{ m_dir.absolutePath() };
    while (!directories.isEmpty()) {
        QString first = directories.takeFirst();
        QDir dir(first);
        for (QFileInfo& fileInfo : dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name)) {
            if (fileInfo.isDir())
                directories.push_back(fileInfo.absoluteFilePath());
            else
                iconFiles.push_back(fileInfo.absoluteFilePath());
        }
    }
    return iconFiles;
}

QString formatName(const QDir& iconsDir, const QFileInfo& file)
{
    if (file.dir() == iconsDir)
        return file.baseName();

    constexpr auto delimiter = " » ";
    QString relativePathWithoutExtension = iconsDir.relativeFilePath(file.dir().path()) + QDir::separator() + file.baseName();
    return relativePathWithoutExtension.replace(QDir::separator(), delimiter);
}

QSet<QString> toStringSet(const QList<QString>& list) // Split into a separate function because the preprocessing impedes readability
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QSet<QString> set(list.begin(), list.end());
#else
    auto set = list.toSet();
#endif
    return set;
}

void IconList::directoryChanged(const QString& path)
{
    QDir newDir(path);
    if (m_dir.absolutePath() != newDir.absolutePath()) {
        m_dir.setPath(path);
        m_dir.refresh();
        if (is_watching)
            stopWatching();
        startWatching();
    }
    if (!m_dir.exists())
        if (!FS::ensureFolderPathExists(m_dir.absolutePath()))
            return;
    m_dir.refresh();
    const QStringList newFileNamesList = getIconFilePaths();
    const QSet<QString> newSet = toStringSet(newFileNamesList);
    QSet<QString> currentSet;
    for (const MMCIcon& it : icons) {
        if (!it.has(IconType::FileBased))
            continue;
        currentSet.insert(it.m_images[IconType::FileBased].filename);
    }
    QSet<QString> toRemove = currentSet - newSet;
    QSet<QString> toAdd = newSet - currentSet;

    for (const auto& remove : toRemove) {
        qDebug() << "Removing " << remove;
        QFileInfo removedFile(remove);
        QString key = m_dir.relativeFilePath(removedFile.absoluteFilePath());

        int idx = getIconIndex(key);
        if (idx == -1)
            continue;
        icons[idx].remove(FileBased);
        if (icons[idx].type() == ToBeDeleted) {
            beginRemoveRows(QModelIndex(), idx, idx);
            icons.remove(idx);
            reindex();
            endRemoveRows();
        } else {
            dataChanged(index(idx), index(idx));
        }
        m_watcher->removePath(remove);
        emit iconUpdated(key);
    }

    for (const auto& add : toAdd) {
        qDebug() << "Adding " << add;

        QFileInfo addfile(add);
        QString key = m_dir.relativeFilePath(addfile.absoluteFilePath());
        QString name = formatName(m_dir, addfile);

        if (addIcon(key, name, addfile.filePath(), IconType::FileBased)) {
            m_watcher->addPath(add);
            emit iconUpdated(key);
        }
    }

    sortIconList();
}

void IconList::fileChanged(const QString& path)
{
    qDebug() << "Checking " << path;
    QFileInfo checkfile(path);
    if (!checkfile.exists())
        return;
    QString key = m_dir.relativeFilePath(checkfile.absoluteFilePath());
    int idx = getIconIndex(key);
    if (idx == -1)
        return;
    QIcon icon(path);
    if (icon.availableSizes().empty())
        return;

    icons[idx].m_images[IconType::FileBased].icon = icon;
    dataChanged(index(idx), index(idx));
    emit iconUpdated(key);
}

void IconList::SettingChanged(const Setting& setting, const QVariant& value)
{
    if (setting.id() != "IconsDir")
        return;

    directoryChanged(value.toString());
}

void IconList::startWatching()
{
    auto abs_path = m_dir.absolutePath();
    FS::ensureFolderPathExists(abs_path);
    is_watching = addPathRecursively(abs_path);
    if (is_watching) {
        qDebug() << "Started watching " << abs_path;
    } else {
        qDebug() << "Failed to start watching " << abs_path;
    }
}

void IconList::stopWatching()
{
    m_watcher->removePaths(m_watcher->files());
    m_watcher->removePaths(m_watcher->directories());
    is_watching = false;
}

QStringList IconList::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}
Qt::DropActions IconList::supportedDropActions() const
{
    return Qt::CopyAction;
}

bool IconList::dropMimeData(const QMimeData* data,
                            Qt::DropAction action,
                            [[maybe_unused]] int row,
                            [[maybe_unused]] int column,
                            [[maybe_unused]] const QModelIndex& parent)
{
    if (action == Qt::IgnoreAction)
        return true;
    // check if the action is supported
    if (!data || !(action & supportedDropActions()))
        return false;

    // files dropped from outside?
    if (data->hasUrls()) {
        auto urls = data->urls();
        QStringList iconFiles;
        for (const auto& url : urls) {
            // only local files may be dropped...
            if (!url.isLocalFile())
                continue;
            iconFiles += url.toLocalFile();
        }
        installIcons(iconFiles);
        return true;
    }
    return false;
}

Qt::ItemFlags IconList::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    return Qt::ItemIsDropEnabled | defaultFlags;
}

QVariant IconList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    int row = index.row();

    if (row < 0 || row >= icons.size())
        return {};

    switch (role) {
        case Qt::DecorationRole:
            return icons[row].icon();
        case Qt::DisplayRole:
            return icons[row].name();
        case Qt::UserRole:
            return icons[row].m_key;
        default:
            return {};
    }
}

int IconList::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : icons.size();
}

void IconList::installIcons(const QStringList& iconFiles)
{
    for (const QString& file : iconFiles)
        installIcon(file, {});
}

void IconList::installIcon(const QString& file, const QString& name)
{
    QFileInfo fileinfo(file);
    if (!fileinfo.isReadable() || !fileinfo.isFile())
        return;

    if (!IconUtils::isIconSuffix(fileinfo.suffix()))
        return;

    QString target = FS::PathCombine(getDirectory(), name.isEmpty() ? fileinfo.fileName() : name);
    QFile::copy(file, target);
}

bool IconList::iconFileExists(const QString& key) const
{
    auto iconEntry = icon(key);
    return iconEntry && iconEntry->has(IconType::FileBased);
}

/// Returns the icon with the given key or nullptr if it doesn't exist.
const MMCIcon* IconList::icon(const QString& key) const
{
    int iconIdx = getIconIndex(key);
    if (iconIdx == -1)
        return nullptr;
    return &icons[iconIdx];
}

bool IconList::deleteIcon(const QString& key)
{
    return iconFileExists(key) && FS::deletePath(icon(key)->getFilePath());
}

bool IconList::trashIcon(const QString& key)
{
    return iconFileExists(key) && FS::trash(icon(key)->getFilePath(), nullptr);
}

bool IconList::addThemeIcon(const QString& key)
{
    auto iter = name_index.find(key);
    if (iter != name_index.end()) {
        auto& oldOne = icons[*iter];
        oldOne.replace(Builtin, key);
        dataChanged(index(*iter), index(*iter));
        return true;
    }
    // add a new icon
    beginInsertRows(QModelIndex(), icons.size(), icons.size());
    {
        MMCIcon mmc_icon;
        mmc_icon.m_name = key;
        mmc_icon.m_key = key;
        mmc_icon.replace(Builtin, key);
        icons.push_back(mmc_icon);
        name_index[key] = icons.size() - 1;
    }
    endInsertRows();
    return true;
}

bool IconList::addIcon(const QString& key, const QString& name, const QString& path, const IconType type)
{
    // replace the icon even? is the input valid?
    QIcon icon(path);
    if (icon.isNull())
        return false;
    auto iter = name_index.find(key);
    if (iter != name_index.end()) {
        auto& oldOne = icons[*iter];
        oldOne.replace(type, icon, path);
        dataChanged(index(*iter), index(*iter));
        return true;
    }
    // add a new icon
    beginInsertRows(QModelIndex(), icons.size(), icons.size());
    {
        MMCIcon mmc_icon;
        mmc_icon.m_name = name;
        mmc_icon.m_key = key;
        mmc_icon.replace(type, icon, path);
        icons.push_back(mmc_icon);
        name_index[key] = icons.size() - 1;
    }
    endInsertRows();
    return true;
}

void IconList::saveIcon(const QString& key, const QString& path, const char* format) const
{
    auto icon = getIcon(key);
    auto pixmap = icon.pixmap(128, 128);
    pixmap.save(path, format);
}

void IconList::reindex()
{
    name_index.clear();
    for (int i = 0; i < icons.size(); i++) {
        name_index[icons[i].m_key] = i;
        emit iconUpdated(icons[i].m_key);  // prevents incorrect indices with proxy model
    }
}

QIcon IconList::getIcon(const QString& key) const
{
    int iconIndex = getIconIndex(key);

    if (iconIndex != -1)
        return icons[iconIndex].icon();

    // Fallback for icons that don't exist.b
    iconIndex = getIconIndex("grass");

    if (iconIndex != -1)
        return icons[iconIndex].icon();
    return {};
}

int IconList::getIconIndex(const QString& key) const
{
    auto iter = name_index.find(key == "default" ? "grass" : key);
    if (iter != name_index.end())
        return *iter;

    return -1;
}

QString IconList::getDirectory() const
{
    return m_dir.absolutePath();
}

/// Returns the directory of the icon with the given key or the default directory if it's a builtin icon.
QString IconList::iconDirectory(const QString& key) const
{
    for (const auto& mmcIcon : icons) {
        if (mmcIcon.m_key == key && mmcIcon.has(IconType::FileBased)) {
            QFileInfo icon_file(mmcIcon.getFilePath());
            return icon_file.dir().path();
        }
    }
    return getDirectory();
}
