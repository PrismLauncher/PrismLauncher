// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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
#include "ui_ExternalResourcesPage.h"

#include "minecraft/mod/TexturePackFolderModel.h"
#include "minecraft/mod/TexturePack.h"

class TexturePackPage : public ExternalResourcesPage
{
    Q_OBJECT
public:
    explicit TexturePackPage(MinecraftInstance *instance, std::shared_ptr<TexturePackFolderModel> model, QWidget *parent = 0)
        : ExternalResourcesPage(instance, model, parent)
    {
        ui->actionViewConfigs->setVisible(false);
    }
    virtual ~TexturePackPage() {}

    QString displayName() const override { return tr("Texture packs"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("resourcepacks"); }
    QString id() const override { return "texturepacks"; }
    QString helpPage() const override { return "Texture-packs"; }

    virtual bool shouldDisplay() const override
    {
        return m_instance->traits().contains("texturepacks");
    }

   public slots:
    bool onSelectionChanged(const QModelIndex& current, const QModelIndex& previous) override
    {
        auto sourceCurrent = m_filterModel->mapToSource(current);
        int row = sourceCurrent.row();
        auto& rp = static_cast<TexturePack&>(m_model->at(row));
        ui->frame->updateWithTexturePack(rp);

        return true;
    }
};
