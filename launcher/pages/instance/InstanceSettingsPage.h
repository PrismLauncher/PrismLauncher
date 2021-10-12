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

#include "java/JavaChecker.h"
#include "BaseInstance.h"
#include <QObjectPtr.h>
#include "pages/BasePage.h"
#include "JavaCommon.h"
#include "Launcher.h"

class JavaChecker;
namespace Ui
{
class InstanceSettingsPage;
}

class InstanceSettingsPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit InstanceSettingsPage(BaseInstance *inst, QWidget *parent = 0);
    virtual ~InstanceSettingsPage();
    virtual QString displayName() const override
    {
        return tr("Settings");
    }
    virtual QIcon icon() const override
    {
        return LAUNCHER->getThemedIcon("instance-settings");
    }
    virtual QString id() const override
    {
        return "settings";
    }
    virtual bool apply() override;
    virtual QString helpPage() const override
    {
        return "Instance-settings";
    }
    virtual bool shouldDisplay() const override;

private slots:
    void on_javaDetectBtn_clicked();
    void on_javaTestBtn_clicked();
    void on_javaBrowseBtn_clicked();

    void applySettings();
    void loadSettings();

    void checkerFinished();

    void globalSettingsButtonClicked(bool checked);

private:
    Ui::InstanceSettingsPage *ui;
    BaseInstance *m_instance;
    SettingsObjectPtr m_settings;
    unique_qobject_ptr<JavaCommon::TestCheck> checker;
};
