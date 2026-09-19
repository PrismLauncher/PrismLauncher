// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 flowln <flowlnlnln@gmail.com>
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

#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QVariant>

#include "Result.h"

// Sectionless INI parser (for instance config files)
class INIFile : public QMap<QString, QVariant> {
   public:
    explicit INIFile();

    Result<> loadFile(const QString& fileName);

    Result<> loadFile(const QByteArray& data);

    Result<> saveFile(const QString& fileName);

    QVariant get(const QString& key, QVariant def) const;
    void set(const QString& key, QVariant val);

    template <typename T>
    T convert(const QString& key, T defaultValue = {}) const
    {
        QVariant val = value(key);
        if (!val.isValid()) {
            return defaultValue;
        }

        if (!val.convert(QMetaType::fromType<T>())) {
            return defaultValue;
        }

        return val.value<T>();
    }

    // NOTE: conversion to const char* doesn't work, even though QMetaType::fromType does
    const char* convert(const QString& key, const char* defaultValue) const = delete;
};
