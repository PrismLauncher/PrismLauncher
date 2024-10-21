// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "ExternalResourcesPage.h"

class ShaderPackPage : public ExternalResourcesPage {
    Q_OBJECT
   public:
    explicit ShaderPackPage(MinecraftInstance* instance, std::shared_ptr<ShaderPackFolderModel> model, QWidget* parent = nullptr);
    ~ShaderPackPage() override = default;

    QString displayName() const override { return tr("Shader packs"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("shaderpacks"); }
    QString id() const override { return "shaderpacks"; }
    QString helpPage() const override { return "shader-packs"; }

    bool shouldDisplay() const override { return true; }

   public slots:
    void downloadShaderPack();
    void updateShaderPacks();
    void deleteShaderPackMetadata();

   private:
    std::shared_ptr<ShaderPackFolderModel> m_model;
};
