// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#include "minecraft/mod/ModFolderModel.h"

#include "ui/dialogs/ResourceDownloadDialog.h"

class QDialogButtonBox;

class ModDownloadDialog final : public ResourceDownloadDialog
{
    Q_OBJECT

   public:
    explicit ModDownloadDialog(QWidget* parent, const std::shared_ptr<ModFolderModel>& mods, BaseInstance* instance);
    ~ModDownloadDialog() override = default;

    //: String that gets appended to the mod download dialog title ("Download " + resourcesString())
    [[nodiscard]] QString resourceString() const override { return tr("mods"); }

    QList<BasePage*> getPages() override;

   public slots:
    void accept() override;
    void reject() override;

   private:
    BaseInstance* m_instance;
};
