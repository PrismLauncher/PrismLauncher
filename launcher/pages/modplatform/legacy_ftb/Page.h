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
#include <QTreeView>
#include <QTextBrowser>

#include "pages/BasePage.h"
#include <Application.h>
#include "tasks/Task.h"
#include "modplatform/legacy_ftb/PackHelpers.h"
#include "modplatform/legacy_ftb/PackFetchTask.h"
#include "QObjectPtr.h"

class NewInstanceDialog;

namespace LegacyFTB {

namespace Ui
{
class Page;
}

class ListModel;
class FilterModel;
class PrivatePackListModel;
class PrivatePackFilterModel;
class PrivatePackManager;

class Page : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit Page(NewInstanceDialog * dialog, QWidget *parent = 0);
    virtual ~Page();
    QString displayName() const override
    {
        return tr("FTB Legacy");
    }
    QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("ftb_logo");
    }
    QString id() const override
    {
        return "legacy_ftb";
    }
    QString helpPage() const override
    {
        return "FTB-platform";
    }
    bool shouldDisplay() const override;
    void openedImpl() override;

private:
    void suggestCurrent();
    void onPackSelectionChanged(Modpack *pack = nullptr);

private slots:
    void ftbPackDataDownloadSuccessfully(ModpackList publicPacks, ModpackList thirdPartyPacks);
    void ftbPackDataDownloadFailed(QString reason);

    void ftbPrivatePackDataDownloadSuccessfully(Modpack pack);
    void ftbPrivatePackDataDownloadFailed(QString reason, QString packCode);

    void onSortingSelectionChanged(QString data);
    void onVersionSelectionItemChanged(QString data);

    void onPublicPackSelectionChanged(QModelIndex first, QModelIndex second);
    void onThirdPartyPackSelectionChanged(QModelIndex first, QModelIndex second);
    void onPrivatePackSelectionChanged(QModelIndex first, QModelIndex second);

    void onTabChanged(int tab);

    void onAddPackClicked();
    void onRemovePackClicked();

private:
    FilterModel* currentModel = nullptr;
    QTreeView* currentList = nullptr;
    QTextBrowser* currentModpackInfo = nullptr;

    bool initialized = false;
    Modpack selected;
    QString selectedVersion;

    ListModel* publicListModel = nullptr;
    FilterModel* publicFilterModel = nullptr;

    ListModel *thirdPartyModel = nullptr;
    FilterModel *thirdPartyFilterModel = nullptr;

    ListModel *privateListModel = nullptr;
    FilterModel *privateFilterModel = nullptr;

    unique_qobject_ptr<PackFetchTask> ftbFetchTask;
    std::unique_ptr<PrivatePackManager> ftbPrivatePacks;

    NewInstanceDialog* dialog = nullptr;

    Ui::Page *ui = nullptr;
};

}
