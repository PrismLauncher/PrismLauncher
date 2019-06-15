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

#include <QWidget>

#include "minecraft/MinecraftInstance.h"
#include "minecraft/ComponentList.h"
#include "pages/BasePage.h"

namespace Ui
{
class VersionPage;
}

class VersionPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit VersionPage(MinecraftInstance *inst, QWidget *parent = 0);
    virtual ~VersionPage();
    virtual QString displayName() const override
    {
        return tr("Version");
    }
    virtual QIcon icon() const override;
    virtual QString id() const override
    {
        return "version";
    }
    virtual QString helpPage() const override
    {
        return "Instance-Version";
    }
    virtual bool shouldDisplay() const override;

private slots:
    void on_fabricBtn_clicked();
    void on_forgeBtn_clicked();
    void on_addEmptyBtn_clicked();
    void on_liteloaderBtn_clicked();
    void on_reloadBtn_clicked();
    void on_removeBtn_clicked();
    void on_moveUpBtn_clicked();
    void on_moveDownBtn_clicked();
    void on_jarmodBtn_clicked();
    void on_jarBtn_clicked();
    void on_revertBtn_clicked();
    void on_editBtn_clicked();
    void on_modBtn_clicked();
    void on_customizeBtn_clicked();
    void on_downloadBtn_clicked();

    void updateVersionControls();
    void disableVersionControls();
    void on_changeVersionBtn_clicked();

private:
    Component * current();
    int currentRow();
    void updateButtons(int row = -1);
    void preselect(int row = 0);
    int doUpdate();

protected:
    /// FIXME: this shouldn't be necessary!
    bool reloadComponentList();

private:
    Ui::VersionPage *ui;
    std::shared_ptr<ComponentList> m_profile;
    MinecraftInstance *m_inst;
    int currentIdx = 0;

public slots:
    void versionCurrent(const QModelIndex &current, const QModelIndex &previous);

private slots:
    void onGameUpdateError(QString error);
    void packageCurrent(const QModelIndex &current, const QModelIndex &previous);

};
