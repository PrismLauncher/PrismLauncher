// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include <memory>

#include <QString>
#include <QStringList>
#include <QUrl>

#include "minecraft/mod/MetadataHandler.h"

struct ModLicense {
    QString name = {};
    QString id = {};
    QString url = {};
    QString description = {};

    ModLicense() {}

    ModLicense(const QString license)
    {
        // FIXME: come up with a better license parsing.
        // handle SPDX identifiers? https://spdx.org/licenses/
        auto parts = license.split(' ');
        QStringList notNameParts = {};
        for (auto part : parts) {
            auto _url = QUrl(part);
            if (part.startsWith("(") && part.endsWith(")"))
                _url = QUrl(part.mid(1, part.size() - 2));

            if (_url.isValid() && !_url.scheme().isEmpty() && !_url.host().isEmpty()) {
                this->url = _url.toString();
                notNameParts.append(part);
                continue;
            }
        }

        for (auto part : notNameParts) {
            parts.removeOne(part);
        }

        auto licensePart = parts.join(' ');
        this->name = licensePart;
        this->description = licensePart;

        if (parts.size() == 1) {
            this->id = parts.first();
        }
    }

    ModLicense(const QString& name_, const QString& id_, const QString& url_, const QString& description_)
        : name(name_), id(id_), url(url_), description(description_)
    {}

    ModLicense(const ModLicense& other) : name(other.name), id(other.id), url(other.url), description(other.description) {}

    ModLicense& operator=(const ModLicense& other)
    {
        this->name = other.name;
        this->id = other.id;
        this->url = other.url;
        this->description = other.description;

        return *this;
    }

    ModLicense& operator=(const ModLicense&& other)
    {
        this->name = other.name;
        this->id = other.id;
        this->url = other.url;
        this->description = other.description;

        return *this;
    }

    bool isEmpty() { return this->name.isEmpty() && this->id.isEmpty() && this->url.isEmpty() && this->description.isEmpty(); }
};

struct ModDetails {
    /* Mod ID as defined in the ModLoader-specific metadata */
    QString mod_id = {};

    /* Human-readable name */
    QString name = {};

    /* Human-readable mod version */
    QString version = {};

    /* Human-readable minecraft version */
    QString mcversion = {};

    /* URL for mod's home page */
    QString homeurl = {};

    /* Human-readable description */
    QString description = {};

    /* List of the author's names */
    QStringList authors = {};

    /* Issue Tracker URL */
    QString issue_tracker = {};

    /* License */
    QList<ModLicense> licenses = {};

    /* Path of mod logo */
    QString icon_file = {};

    ModDetails() = default;

    /** Metadata should be handled manually to properly set the mod status. */
    ModDetails(const ModDetails& other)
        : mod_id(other.mod_id)
        , name(other.name)
        , version(other.version)
        , mcversion(other.mcversion)
        , homeurl(other.homeurl)
        , description(other.description)
        , authors(other.authors)
        , issue_tracker(other.issue_tracker)
        , licenses(other.licenses)
        , icon_file(other.icon_file)
    {}

    ModDetails& operator=(const ModDetails& other) = default;

    ModDetails& operator=(ModDetails&& other) = default;
};
