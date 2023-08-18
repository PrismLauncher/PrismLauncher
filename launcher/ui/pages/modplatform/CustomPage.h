// SPDX-License-Identifier: GPL-3.0-only
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

#include <QWidget>

#include <Application.h>
#include "tasks/Task.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class CustomPage;
}

class NewInstanceDialog;

class CustomPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit CustomPage(NewInstanceDialog* dialog, QWidget* parent = 0);
    virtual ~CustomPage();
    virtual QString displayName() const override { return tr("Custom"); }
    virtual QIcon icon() const override { return APPLICATION->getThemedIcon("minecraft"); }
    virtual QString id() const override { return "vanilla"; }
    virtual QString helpPage() const override { return "Vanilla-platform"; }
    virtual bool shouldDisplay() const override;
    void retranslate() override;

    void openedImpl() override;

    BaseVersion::Ptr selectedVersion() const;
    BaseVersion::Ptr selectedLoaderVersion() const;
    QString selectedLoader() const;

   public slots:
    void setSelectedVersion(BaseVersion::Ptr version);
    void setSelectedLoaderVersion(BaseVersion::Ptr version);

   private slots:
    void filterChanged();
    void loaderFilterChanged();

   private:
    void refresh();
    void loaderRefresh();
    void suggestCurrent();

   private:
    bool initialized = false;
    NewInstanceDialog* dialog = nullptr;
    Ui::CustomPage* ui = nullptr;
    bool m_versionSetByUser = false;
    BaseVersion::Ptr m_selectedVersion;
    BaseVersion::Ptr m_selectedLoaderVersion;
    QString m_selectedLoader;
};
