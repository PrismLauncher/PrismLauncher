// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "ExternalResourcesPage.h"
#include "minecraft/mod/DataPackFolderModel.h"
#include "ui_ExternalResourcesPage.h"

class DataPackPage : public ExternalResourcesPage {
    Q_OBJECT
   public:
    explicit DataPackPage(MinecraftInstance* instance, std::shared_ptr<DataPackFolderModel> model, QWidget* parent = 0);

    QString displayName() const override { return tr("Data packs"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("datapacks"); }
    QString id() const override { return "datapacks"; }
    QString helpPage() const override { return "Data-packs"; }
    bool shouldDisplay() const override { return true; }

   public slots:
    bool onSelectionChanged(const QModelIndex& current, const QModelIndex& previous) override;
    void downloadDataPacks();
};
