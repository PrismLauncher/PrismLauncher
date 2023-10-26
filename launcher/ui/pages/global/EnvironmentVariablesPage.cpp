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

#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include "EnvironmentVariablesPage.h"

EnvironmentVariablesPage::EnvironmentVariablesPage(QWidget* parent) : QWidget(parent)
{
    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    auto tabWidget = new QTabWidget(this);
    tabWidget->setObjectName(QStringLiteral("tabWidget"));
    variables = new EnvironmentVariables(this);
    variables->setContentsMargins(6, 6, 6, 6);
    tabWidget->addTab(variables, "Foo");
    tabWidget->tabBar()->hide();
    verticalLayout->addWidget(tabWidget);

    variables->initialize(false, false, APPLICATION->settings()->get("Env").toMap());
}

QString EnvironmentVariablesPage::displayName() const
{
    return tr("Environment Variables");
}

QIcon EnvironmentVariablesPage::icon() const
{
    return APPLICATION->getThemedIcon("environment-variables");
}

QString EnvironmentVariablesPage::id() const
{
    return "environment-variables";
}

QString EnvironmentVariablesPage::helpPage() const
{
    return "Environment-variables";
}

bool EnvironmentVariablesPage::apply()
{
    APPLICATION->settings()->set("Env", variables->value());
    return true;
}

void EnvironmentVariablesPage::retranslate()
{
    variables->retranslate();
}
