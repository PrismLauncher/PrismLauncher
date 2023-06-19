// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include <QDialog>
#include "ui/pages/BasePageProvider.h"

class MinecraftInstance;
class PageContainer;
class PackProfile;

class InstallLoaderDialog : public QDialog, public BasePageProvider {
    Q_OBJECT

   public:
    explicit InstallLoaderDialog(std::shared_ptr<PackProfile> instance, QWidget* parent = nullptr);

    QList<BasePage*> getPages() override;
    QString dialogTitle() override;

    void done(int result) override;

   private:
    std::shared_ptr<PackProfile> m_profile;
    PageContainer* m_container;
};
