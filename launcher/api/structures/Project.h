// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023-2025 Trial97 <alexandru.tripon97@gmail.com>
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

#include <memory>

#include <QString>
#include <QVariant>
#include <memory>
#include "api/structures/Provider.h"
#include "api/structures/Side.h"
#include "api/structures/Version.h"

namespace Platform {

struct ModpackAuthor {
    QString name;
    QString url;
};

struct DonationData {
    QString id;
    QString platform;
    QString url;
};

struct ExtraPackData {
    QList<DonationData> donate;

    QString issuesUrl;
    QString sourceUrl;
    QString wikiUrl;
    QString discordUrl;

    QString status;

    QString body;
};

struct Project {
    using Ptr = std::shared_ptr<Project>;

    QVariant projectId;
    Provider provider;
    QString name;
    QString slug;
    QString description;
    QList<ModpackAuthor> authors;
    QString logoName;
    QString logoUrl;
    QString websiteUrl;
    Side side;

    bool versionsLoaded = false;
    QList<Version> versions;

    // Don't load by default, since some modplatform don't have that info
    bool extraDataLoaded = true;
    ExtraPackData extraData;

    // For internal use, not provided by APIs
    [[nodiscard]] bool isVersionSelected(int index) const
    {
        if (!versionsLoaded)
            return false;

        return versions.at(index).is_currently_selected;
    }
    [[nodiscard]] bool isAnyVersionSelected() const
    {
        if (!versionsLoaded)
            return false;

        return std::any_of(versions.constBegin(), versions.constEnd(), [](auto const& v) { return v.is_currently_selected; });
    }
};
}  // namespace Platform

Q_DECLARE_METATYPE(Platform::Project)
Q_DECLARE_METATYPE(Platform::Project::Ptr)
