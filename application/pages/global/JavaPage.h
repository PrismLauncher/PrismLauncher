/* Copyright 2013-2018 MultiMC Contributors
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

#include <memory>
#include <QDialog>
#include "pages/BasePage.h"
#include "JavaCommon.h"
#include <MultiMC.h>
#include <QObjectPtr.h>

class SettingsObject;

namespace Ui
{
class JavaPage;
}

class JavaPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit JavaPage(QWidget *parent = 0);
    ~JavaPage();

    QString displayName() const override
    {
        return tr("Java");
    }
    QIcon icon() const override
    {
        return MMC->getThemedIcon("java");
    }
    QString id() const override
    {
        return "java-settings";
    }
    QString helpPage() const override
    {
        return "Java-settings";
    }
    bool apply() override;

private:
    void applySettings();
    void loadSettings();

private
slots:
    void on_javaDetectBtn_clicked();
    void on_javaTestBtn_clicked();
    void on_javaBrowseBtn_clicked();
    void checkerFinished();

private:
    Ui::JavaPage *ui;
    unique_qobject_ptr<JavaCommon::TestCheck> checker;
};
