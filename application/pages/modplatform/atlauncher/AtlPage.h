/* Copyright 2013-2019 MultiMC Contributors
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

#include "AtlFilterModel.h"
#include "AtlListModel.h"

#include <QWidget>
#include <modplatform/atlauncher/ATLPackInstallTask.h>

#include "MultiMC.h"
#include "pages/BasePage.h"
#include "tasks/Task.h"

namespace Ui
{
    class AtlPage;
}

class NewInstanceDialog;

class AtlPage : public QWidget, public BasePage, public ATLauncher::UserInteractionSupport
{
Q_OBJECT

public:
    explicit AtlPage(NewInstanceDialog* dialog, QWidget *parent = 0);
    virtual ~AtlPage();
    virtual QString displayName() const override
    {
        return tr("ATLauncher");
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon("atlauncher");
    }
    virtual QString id() const override
    {
        return "atl";
    }
    virtual QString helpPage() const override
    {
        return "ATL-platform";
    }
    virtual bool shouldDisplay() const override;

    void openedImpl() override;

private:
    void suggestCurrent();

    QString chooseVersion(Meta::VersionListPtr vlist, QString minecraftVersion) override;

private slots:
    void triggerSearch();
    void resetSearch();

    void onSortingSelectionChanged(QString data);

    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);

private:
    Ui::AtlPage *ui = nullptr;
    NewInstanceDialog* dialog = nullptr;
    Atl::ListModel* listModel = nullptr;
    Atl::FilterModel* filterModel = nullptr;

    ATLauncher::IndexedPack selected;
    QString selectedVersion;
};
