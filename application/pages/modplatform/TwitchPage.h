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

#include "pages/BasePage.h"
#include <MultiMC.h>
#include "tasks/Task.h"
#include "modplatform/flame/UrlResolvingTask.h"

namespace Ui
{
class TwitchPage;
}

class NewInstanceDialog;

class TwitchPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit TwitchPage(NewInstanceDialog* dialog, QWidget *parent = 0);
    virtual ~TwitchPage();
    virtual QString displayName() const override
    {
        return tr("Twitch");
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon("twitch");
    }
    virtual QString id() const override
    {
        return "twitch";
    }
    virtual QString helpPage() const override
    {
        return "Twitch-platform";
    }
    virtual bool shouldDisplay() const override;

    void openedImpl() override;

private slots:
    void triggerCheck(bool checked);
    void checkDone();

private:
    Ui::TwitchPage *ui = nullptr;
    NewInstanceDialog* dialog = nullptr;
    shared_qobject_ptr<Flame::UrlResolvingTask> m_modIdResolver;
};
