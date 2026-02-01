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

#include <QIcon>
#include <memory>
#include "modplatform/ftb/FTBPackManifest.h"
#include "net/NetJob.h"

namespace Ftb {

struct Logo {
    QString fullpath;
    NetJob::Ptr downloadJob;
    QIcon result;
    bool failed = false;
};

using LogoMap = QMap<QString, Logo>;
using LogoCallback = std::function<void(QString)>;

class ListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ListModel(QObject* parent);
    virtual ~ListModel();

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void request();
    void abortRequest();

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);

    [[nodiscard]] bool isMakingRequest() const { return m_jobPtr.get(); }
    [[nodiscard]] bool wasAborted() const { return m_aborted; }

   private slots:
    void requestFinished();
    void requestFailed(QString reason);

    void requestPack();
    void packRequestFinished();
    void packRequestFailed(QString reason);

    void logoFailed(QString logo);
    void logoLoaded(QString logo);

   private:
    void requestLogo(QString file, QString url);

   private:
    bool m_aborted = false;

    QList<FTB::Modpack> m_modpacks;
    LogoMap m_logoMap;

    NetJob::Ptr m_jobPtr;
    int m_currentPack;
    QList<int> m_remainingPacks;
    std::unique_ptr<QByteArray> m_response;
};

}  // namespace Ftb
