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

#include <QMainWindow>

#include "minecraft/MinecraftInstance.h"
#include "pages/BasePage.h"
#include <MultiMC.h>

class SimpleModList;
namespace Ui
{
class ModFolderPage;
}

class ModFolderPage : public QMainWindow, public BasePage
{
    Q_OBJECT

public:
    explicit ModFolderPage(
        BaseInstance *inst,
        std::shared_ptr<SimpleModList> mods,
        QString id,
        QString iconName,
        QString displayName,
        QString helpPage = "",
        QWidget *parent = 0
    );
    virtual ~ModFolderPage();

    void setFilter(const QString & filter)
    {
        m_fileSelectionFilter = filter;
    }

    virtual QString displayName() const override
    {
        return m_displayName;
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon(m_iconName);
    }
    virtual QString id() const override
    {
        return m_id;
    }
    virtual QString helpPage() const override
    {
        return m_helpName;
    }
    virtual bool shouldDisplay() const override;

    virtual void openedImpl() override;
    virtual void closedImpl() override;
protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    bool modListFilter(QKeyEvent *ev);
    QMenu * createPopupMenu() override;

protected:
    BaseInstance *m_inst = nullptr;

protected:
    Ui::ModFolderPage *ui = nullptr;
    std::shared_ptr<SimpleModList> m_mods;
    QSortFilterProxyModel *m_filterModel = nullptr;
    QString m_iconName;
    QString m_id;
    QString m_displayName;
    QString m_helpName;
    QString m_fileSelectionFilter;
    QString m_viewFilter;
    bool m_controlsEnabled = true;

public
slots:
    void modCurrent(const QModelIndex &current, const QModelIndex &previous);

private
slots:
    void on_filterTextChanged(const QString & newContents);
    void on_RunningState_changed(bool running);
    void on_actionAdd_triggered();
    void on_actionRemove_triggered();
    void on_actionEnable_triggered();
    void on_actionDisable_triggered();
    void on_actionView_Folder_triggered();
    void on_actionView_configs_triggered();
};

class CoreModFolderPage : public ModFolderPage
{
public:
    explicit CoreModFolderPage(BaseInstance *inst, std::shared_ptr<SimpleModList> mods, QString id,
                               QString iconName, QString displayName, QString helpPage = "",
                               QWidget *parent = 0);
    virtual ~CoreModFolderPage()
    {
    }
    virtual bool shouldDisplay() const;
};
