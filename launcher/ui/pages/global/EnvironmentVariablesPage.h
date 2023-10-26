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

#include <Application.h>

#include "ui/pages/BasePage.h"
#include "ui/widgets/EnvironmentVariables.h"

class EnvironmentVariablesPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit EnvironmentVariablesPage(QWidget* parent = nullptr);

    QString displayName() const override;
    QIcon icon() const override;
    QString id() const override;
    QString helpPage() const override;

    bool apply() override;
    void retranslate() override;

   private:
    EnvironmentVariables* variables;
};
