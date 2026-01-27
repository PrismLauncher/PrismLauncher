// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Prism Launcher Contributors
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

#include "TechnicAPI.h"

#include "Application.h"
#include "BuildConfig.h"
#include "net/ApiDownload.h"

namespace Technic {

QString API::getPlatformPackUrl(const QString& packSlug)
{
    return QString("%1modpack/%2?build=%3").arg(BuildConfig.TECHNIC_API_BASE_URL, packSlug, BuildConfig.TECHNIC_API_BUILD);
}

QString API::getSolderPackUrl(const QString& solderUrl, const QString& packSlug)
{
    QString baseUrl = solderUrl;
    // Ensure no trailing slash
    while (baseUrl.endsWith('/'))
        baseUrl.chop(1);

    return QString("%1/modpack/%2").arg(baseUrl, packSlug);
}

QString API::getSolderBuildUrl(const QString& solderUrl, const QString& packSlug, const QString& build)
{
    QString baseUrl = solderUrl;
    // Ensure no trailing slash
    while (baseUrl.endsWith('/'))
        baseUrl.chop(1);

    return QString("%1/modpack/%2/%3").arg(baseUrl, packSlug, build);
}

Task::Ptr API::getPackInfo(const QString& packSlug, QByteArray* response)
{
    auto netJob = makeShared<NetJob>(QString("Technic::PackInfo(%1)").arg(packSlug), APPLICATION->network());
    netJob->addNetAction(Net::ApiDownload::makeByteArray(getPlatformPackUrl(packSlug), response));
    return netJob;
}

Task::Ptr API::getSolderPackInfo(const QString& solderUrl, const QString& packSlug, QByteArray* response)
{
    auto netJob = makeShared<NetJob>(QString("Technic::SolderPackInfo(%1)").arg(packSlug), APPLICATION->network());
    netJob->addNetAction(Net::ApiDownload::makeByteArray(getSolderPackUrl(solderUrl, packSlug), response));
    return netJob;
}

Task::Ptr API::getSolderPackBuild(const QString& solderUrl, const QString& packSlug, const QString& build, QByteArray* response)
{
    auto netJob = makeShared<NetJob>(QString("Technic::SolderBuild(%1/%2)").arg(packSlug, build), APPLICATION->network());
    netJob->addNetAction(Net::ApiDownload::makeByteArray(getSolderBuildUrl(solderUrl, packSlug, build), response));
    return netJob;
}

}  // namespace Technic
