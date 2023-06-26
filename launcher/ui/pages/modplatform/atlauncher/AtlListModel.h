/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
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

#pragma once

#include <QAbstractListModel>

#include <modplatform/atlauncher/ATLPackIndex.h>
#include <QIcon>
#include "net/NetJob.h"

namespace Atl {

typedef QMap<QString, QIcon> LogoMap;
typedef std::function<void(QString)> LogoCallback;

class ListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ListModel(QObject* parent);
    virtual ~ListModel();

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void request();

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);

   private slots:
    void requestFinished();
    void requestFailed(QString reason);

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

   private:
    void requestLogo(QString file, QString url);

   private:
    QList<ATLauncher::IndexedPack> modpacks;

    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    LogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    NetJob::Ptr jobPtr;
    std::shared_ptr<QByteArray> response = std::make_shared<QByteArray>();
};

}  // namespace Atl
