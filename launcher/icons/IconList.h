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
#pragma once

#include <QAbstractListModel>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QtGui/QIcon>
#include <memory>

#include "MMCIcon.h"
#include "settings/Setting.h"

#include "QObjectPtr.h"

class QFileSystemWatcher;

class IconList : public QAbstractListModel {
    Q_OBJECT
   public:
    explicit IconList(const QStringList& builtinPaths, QString path, QObject* parent = 0);
    virtual ~IconList() {};

    QIcon getIcon(const QString& key) const;
    int getIconIndex(const QString& key) const;
    QString getDirectory() const;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual QStringList mimeTypes() const override;
    virtual Qt::DropActions supportedDropActions() const override;
    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool addThemeIcon(const QString& key);
    bool addIcon(const QString& key, const QString& name, const QString& path, IconType type);
    void saveIcon(const QString& key, const QString& path, const char* format) const;
    bool deleteIcon(const QString& key);
    bool trashIcon(const QString& key);
    bool iconFileExists(const QString& key) const;

    void installIcons(const QStringList& iconFiles);
    void installIcon(const QString& file, const QString& name);

    const MMCIcon* icon(const QString& key) const;

    void startWatching();
    void stopWatching();

   signals:
    void iconUpdated(QString key);

   private:
    // hide copy constructor
    IconList(const IconList&) = delete;
    // hide assign op
    IconList& operator=(const IconList&) = delete;
    void reindex();
    void sortIconList();

   public slots:
    void directoryChanged(const QString& path);

   protected slots:
    void fileChanged(const QString& path);
    void SettingChanged(const Setting& setting, QVariant value);

   private:
    shared_qobject_ptr<QFileSystemWatcher> m_watcher;
    bool is_watching;
    QMap<QString, int> name_index;
    QVector<MMCIcon> icons;
    QDir m_dir;
};
