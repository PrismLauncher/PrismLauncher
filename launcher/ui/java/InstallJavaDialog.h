// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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
#include "BaseInstance.h"
#include "ui/pages/BasePageProvider.h"

class MinecraftInstance;
class PageContainer;
class PackProfile;
class QDialogButtonBox;

namespace Java {
class InstallDialog final : public QDialog, private BasePageProvider {
    Q_OBJECT

   public:
    explicit InstallDialog(const QString& uid = QString(), BaseInstance* instance = nullptr, QWidget* parent = nullptr);

    QList<BasePage*> getPages() override;
    QString dialogTitle() override;

    void validate(BasePage* selected);
    void done(int result) override;

   private:
    PageContainer* container;
    QDialogButtonBox* buttons;
};
}  // namespace Java
