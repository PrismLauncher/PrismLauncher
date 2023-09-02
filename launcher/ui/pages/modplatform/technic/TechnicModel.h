// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2021 Jamie Mansfield <jmansfield@cadixdev.org>
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
 *      Copyright 2020-2021 MultiMC Contributors
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

#include <QModelIndex>

#include "TechnicData.h"
#include "net/NetJob.h"

namespace Technic {

typedef std::function<void(QString)> LogoCallback;

class ListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ListModel(QObject* parent);
    virtual ~ListModel();

    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual int rowCount(const QModelIndex& parent) const;

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);
    void searchWithTerm(const QString& term);

    [[nodiscard]] bool hasActiveSearchJob() const { return jobPtr && jobPtr->isRunning(); }
    [[nodiscard]] Task::Ptr activeSearchJob() { return hasActiveSearchJob() ? jobPtr : nullptr; }

   private slots:
    void searchRequestFinished();
    void searchRequestFailed();

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QString out);

   private:
    void performSearch();
    void requestLogo(QString logo, QString url);

   private:
    QList<Modpack> modpacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    QMap<QString, QIcon> m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    QString currentSearchTerm;
    enum SearchState { None, ResetRequested, Finished } searchState = None;
    enum SearchMode {
        List,
        Single,
    } searchMode = List;
    NetJob::Ptr jobPtr;
    std::shared_ptr<QByteArray> response = std::make_shared<QByteArray>();
};

}  // namespace Technic
