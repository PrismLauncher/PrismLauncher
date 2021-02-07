/* Copyright 2018-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>
#include <QDialog>

#include "pages/BasePage.h"
#include <MultiMC.h>
#include "widgets/CustomCommands.h"

class CustomCommandsPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit CustomCommandsPage(QWidget *parent = 0);
    ~CustomCommandsPage();

    QString displayName() const override
    {
        return tr("Custom Commands");
    }
    QIcon icon() const override
    {
        return MMC->getThemedIcon("custom-commands");
    }
    QString id() const override
    {
        return "custom-commands";
    }
    QString helpPage() const override
    {
        return "Custom-commands";
    }
    bool apply() override;

private:
    void applySettings();
    void loadSettings();
    CustomCommands * commands;
};
