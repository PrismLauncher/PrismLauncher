/* Copyright 2015-2019 MultiMC Contributors
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

#include <QMainWindow>

#include "minecraft/MinecraftInstance.h"
#include "pages/BasePage.h"
#include <MultiMC.h>
#include <LoggedProcess.h>

class WorldList;
namespace Ui
{
class WorldListPage;
}

class WorldListPage : public QMainWindow, public BasePage
{
    Q_OBJECT

public:
    explicit WorldListPage(
        BaseInstance *inst,
        std::shared_ptr<WorldList> worlds,
        QWidget *parent = 0
    );
    virtual ~WorldListPage();

    virtual QString displayName() const override
    {
        return tr("Worlds");
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon("worlds");
    }
    virtual QString id() const override
    {
        return "worlds";
    }
    virtual QString helpPage() const override
    {
        return "Worlds";
    }
    virtual bool shouldDisplay() const override;

    virtual void openedImpl() override;
    virtual void closedImpl() override;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    bool worldListFilter(QKeyEvent *ev);
    QMenu * createPopupMenu() override;

protected:
    BaseInstance *m_inst;

private:
    QModelIndex getSelectedWorld();
    bool isWorldSafe(QModelIndex index);
    bool worldSafetyNagQuestion();
    void mceditError();

private:
    Ui::WorldListPage *ui;
    std::shared_ptr<WorldList> m_worlds;
    unique_qobject_ptr<LoggedProcess> m_mceditProcess;
    bool m_mceditStarting = false;

private slots:
    void on_actionCopy_Seed_triggered();
    void on_actionMCEdit_triggered();
    void on_actionRemove_triggered();
    void on_actionAdd_triggered();
    void on_actionCopy_triggered();
    void on_actionRename_triggered();
    void on_actionRefresh_triggered();
    void on_actionView_Folder_triggered();
    void on_actionDatapacks_triggered();
    void on_actionReset_Icon_triggered();
    void worldChanged(const QModelIndex &current, const QModelIndex &previous);
    void mceditState(LoggedProcess::State state);

    void ShowContextMenu(const QPoint &pos);
};
