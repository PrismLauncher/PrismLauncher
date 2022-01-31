/*
 * Copyright 2013-2021 MultiMC Contributors
 * Copyright 2021-2022 kb1000
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

#include "Application.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/pages/BasePage.h"

#include <QWidget>

namespace Ui
{
    class ModrinthPage;
}

class ModrinthPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit ModrinthPage(NewInstanceDialog *dialog, QWidget *parent = nullptr);
    ~ModrinthPage() override;

    QString displayName() const override
    {
        return tr("Modrinth");
    }
    QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("modrinth");
    }
    QString id() const override
    {
        return "modrinth";
    }

    void openedImpl() override;

    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void triggerSearch();

private:
    Ui::ModrinthPage *ui;
    NewInstanceDialog *dialog;
};
