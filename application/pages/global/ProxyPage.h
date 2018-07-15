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
#include <MultiMC.h>

namespace Ui
{
class ProxyPage;
}

class ProxyPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit ProxyPage(QWidget *parent = 0);
    ~ProxyPage();

    QString displayName() const override
    {
        return tr("Proxy");
    }
    QIcon icon() const override
    {
        return MMC->getThemedIcon("proxy");
    }
    QString id() const override
    {
        return "proxy-settings";
    }
    QString helpPage() const override
    {
        return "Proxy-settings";
    }
    bool apply() override;

private:
    void updateCheckboxStuff();
    void applySettings();
    void loadSettings();

private
slots:
    void proxyChanged(int);

private:
    Ui::ProxyPage *ui;
};
