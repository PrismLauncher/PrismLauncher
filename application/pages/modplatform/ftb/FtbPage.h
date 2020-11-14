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

#include "FtbFilterModel.h"
#include "FtbListModel.h"

#include <QWidget>

#include "MultiMC.h"
#include "pages/BasePage.h"
#include "tasks/Task.h"

namespace Ui
{
    class FtbPage;
}

class NewInstanceDialog;

class FtbPage : public QWidget, public BasePage
{
Q_OBJECT

public:
    explicit FtbPage(NewInstanceDialog* dialog, QWidget *parent = 0);
    virtual ~FtbPage();
    virtual QString displayName() const override
    {
        return tr("FTB");
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon("ftb_logo");
    }
    virtual QString id() const override
    {
        return "ftb";
    }
    virtual QString helpPage() const override
    {
        return "FTB-platform";
    }
    virtual bool shouldDisplay() const override;

    void openedImpl() override;

    bool eventFilter(QObject * watched, QEvent * event) override;

private:
    void suggestCurrent();

private slots:
    void triggerSearch();
    void onSortingSelectionChanged(QString data);
    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);

private:
    Ui::FtbPage *ui = nullptr;
    NewInstanceDialog* dialog = nullptr;
    Ftb::ListModel* listModel = nullptr;
    Ftb::FilterModel* filterModel = nullptr;

    ModpacksCH::Modpack selected;
    QString selectedVersion;
};
