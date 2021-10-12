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

#include "pages/BasePage.h"
#include <Launcher.h>

namespace Ui {
class PasteEEPage;
}

class PasteEEPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit PasteEEPage(QWidget *parent = 0);
    ~PasteEEPage();

    QString displayName() const override
    {
        return tr("Log Upload");
    }
    QIcon icon() const override
    {
        return LAUNCHER->getThemedIcon("log");
    }
    QString id() const override
    {
        return "log-upload";
    }
    QString helpPage() const override
    {
        return "Log-Upload";
    }
    virtual bool apply() override;

private:
    void loadSettings();
    void applySettings();

private slots:
    void textEdited(const QString &text);

private:
    Ui::PasteEEPage *ui;
};
