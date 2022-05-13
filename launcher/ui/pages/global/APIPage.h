// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (c) 2022 Lenny McLennington <lenny@sneed.church>
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

#include <QWidget>

#include "ui/pages/BasePage.h"
#include <Application.h>

namespace Ui {
class APIPage;
}

class APIPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit APIPage(QWidget *parent = 0);
    ~APIPage();

    QString displayName() const override
    {
        return tr("APIs");
    }
    QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("worlds");
    }
    QString id() const override
    {
        return "apis";
    }
    QString helpPage() const override
    {
        return "APIs";
    }
    virtual bool apply() override;
    void retranslate() override;

private:
    int baseURLPasteType;
    void resetBaseURLNote();
    void updateBaseURLNote(int index);
    void updateBaseURLPlaceholder(int index);
    void loadSettings();
    void applySettings();

private:
    Ui::APIPage *ui;
};

