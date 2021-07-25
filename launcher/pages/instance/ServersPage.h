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

#include <QMainWindow>
#include <QString>

#include "pages/BasePage.h"
#include <MultiMC.h>

namespace Ui
{
class ServersPage;
}

struct Server;
class ServersModel;
class MinecraftInstance;

class ServersPage : public QMainWindow, public BasePage
{
    Q_OBJECT

public:
    explicit ServersPage(InstancePtr inst, QWidget *parent = 0);
    virtual ~ServersPage();

    void openedImpl() override;
    void closedImpl() override;

    virtual QString displayName() const override
    {
        return tr("Servers");
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon("unknown_server");
    }
    virtual QString id() const override
    {
        return "servers";
    }
    virtual QString helpPage() const override
    {
        return "Servers-management";
    }

protected:
    QMenu * createPopupMenu() override;

private:
    void updateState();
    void scheduleSave();
    bool saveIsScheduled() const;

private slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void rowsRemoved(const QModelIndex &parent, int first, int last);

    void on_actionAdd_triggered();
    void on_actionRemove_triggered();
    void on_actionMove_Up_triggered();
    void on_actionMove_Down_triggered();
    void on_actionJoin_triggered();

    void on_RunningState_changed(bool running);

    void nameEdited(const QString & name);
    void addressEdited(const QString & address);
    void resourceIndexChanged(int index);\

    void ShowContextMenu(const QPoint &pos);

private: // data
    int currentServer = -1;
    bool m_locked = true;
    Ui::ServersPage *ui = nullptr;
    ServersModel * m_model = nullptr;
    InstancePtr m_inst = nullptr;
};

