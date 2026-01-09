// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Tayou <git@tayou.org>
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

#include <ui/widgets/AppearanceWidget.h>
#include <QHBoxLayout>
#include <QWidget>
#include "BaseWizardPage.h"

class ThemeWizardPage : public BaseWizardPage {
    Q_OBJECT

   public:
    ThemeWizardPage(QWidget* parent = nullptr) : BaseWizardPage(parent)
    {
        auto layout = new QVBoxLayout(this);
        layout->addWidget(&widget);
        layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);

        setTitle(tr("Appearance"));
        setSubTitle(tr("Select theme and icons to use"));
    }

    bool validatePage() override { return true; };
    void retranslate() override { widget.retranslateUi(); }

   private:
    AppearanceWidget widget{ true };
};
