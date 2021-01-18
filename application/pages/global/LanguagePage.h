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

#include <memory>
#include "pages/BasePage.h"
#include <MultiMC.h>
#include <QWidget>

class LanguageSelectionWidget;

class LanguagePage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit LanguagePage(QWidget *parent = 0);
    virtual ~LanguagePage();

    QString displayName() const override
    {
        return tr("Language");
    }
    QIcon icon() const override
    {
        return MMC->getThemedIcon("language");
    }
    QString id() const override
    {
        return "language-settings";
    }
    QString helpPage() const override
    {
        return "Language-settings";
    }
    bool apply() override;

    void changeEvent(QEvent * ) override;

private:
    void applySettings();
    void loadSettings();
    void retranslate();

private:
    LanguageSelectionWidget *mainWidget;
};
