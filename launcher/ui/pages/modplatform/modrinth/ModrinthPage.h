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

#include "ui/pages/BasePage.h"
#include <Application.h>
#include "tasks/Task.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

namespace Ui
{
class ModrinthPage;
}

class ModDownloadDialog;

namespace Modrinth {
    class ListModel;
}

class ModrinthPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit ModrinthPage(ModDownloadDialog* dialog, QWidget *parent = 0);
    virtual ~ModrinthPage();
    virtual QString displayName() const override
    {
        return tr("Modrinth");
    }
    virtual QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("flame");
    }
    virtual QString id() const override
    {
        return "modrinth";
    }
    virtual QString helpPage() const override
    {
        return "Modrinth-platform";
    }
    virtual bool shouldDisplay() const override;

    void openedImpl() override;

    bool eventFilter(QObject * watched, QEvent * event) override;

private:
    void suggestCurrent();

private slots:
    void triggerSearch();
    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);

private:
    Ui::ModrinthPage *ui = nullptr;
    ModDownloadDialog* dialog = nullptr;
    Modrinth::ListModel* listModel = nullptr;
    Modrinth::IndexedPack current;

    QString selectedVersion;
};
