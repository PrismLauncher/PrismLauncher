// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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
 */

#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QWidget>

#include "modplatform/ModIndex.h"

class UrlHandler : public QObject {
    Q_OBJECT
   public:
    explicit UrlHandler(QWidget* parent) : m_parent(parent) {}
    void process(const QList<QUrl>& urls);

   signals:
    void addInstance(const QString& url, const QMap<QString, QString>& extraInfo);

   private:
    void processFile(const QUrl& url,
                     const ModPlatform::IndexedVersion& version = {},
                     const QMap<QString, QString>& extraInfo = {},
                     ModPlatform::ResourceProvider provider = ModPlatform::ResourceProvider::MODRINTH);
    void downloadFile(const QUrl& url,
                      const ModPlatform::IndexedVersion& version = {},
                      const QMap<QString, QString>& extraInfo = {},
                      ModPlatform::ResourceProvider provider = ModPlatform::ResourceProvider::MODRINTH);
    static void handleOauth(const QUrl& url);
    void handleModrinth(const QUrl& url);
    void handleModrinthVersion(const QString& versionID);
    void handleModrinthMod(const QString& id);
    void handleCurseforge(const QUrl& url);
    void handleCurseforge(const QString& addonId, const QString& fileId);
    void handlePrism(const QUrl& url);

   private:
    QWidget* m_parent;
};
