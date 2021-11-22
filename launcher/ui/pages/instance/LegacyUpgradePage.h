/* Copyright 2013-2021 MultiMC Contributors
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

#include <QWidget>

#include "minecraft/legacy/LegacyInstance.h"
#include "ui/pages/BasePage.h"
#include <Application.h>
#include "tasks/Task.h"

namespace Ui
{
class LegacyUpgradePage;
}

class LegacyUpgradePage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit LegacyUpgradePage(InstancePtr inst, QWidget *parent = 0);
    virtual ~LegacyUpgradePage();
    virtual QString displayName() const override
    {
        return tr("Upgrade");
    }
    virtual QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("checkupdate");
    }
    virtual QString id() const override
    {
        return "upgrade";
    }
    virtual QString helpPage() const override
    {
        return "Legacy-upgrade";
    }
    virtual bool shouldDisplay() const override;

private slots:
    void on_upgradeButton_clicked();

private:
    void runModalTask(Task *task);

private:
    Ui::LegacyUpgradePage *ui;
    InstancePtr m_inst;
};
