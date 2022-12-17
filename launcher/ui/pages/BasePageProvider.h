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

#include "ui/pages/BasePage.h"
#include <memory>
#include <functional>

class BasePageProvider
{
public:
    virtual QList<BasePage *> getPages() = 0;
    virtual QString dialogTitle() = 0;
};

class GenericPageProvider : public BasePageProvider
{
    typedef std::function<BasePage *()> PageCreator;
public:
    explicit GenericPageProvider(const QString &dialogTitle)
        : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dialogTitle(dialogTitle)
    {
    }
    virtual ~GenericPageProvider() {}

    QList<BasePage *> getPages() override
    {
        QList<BasePage *> pages;
        for (PageCreator creator : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_creators)
        {
            pages.append(creator());
        }
        return pages;
    }
    QString dialogTitle() override { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dialogTitle; }

    void setDialogTitle(const QString &title)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dialogTitle = title;
    }
    void addPageCreator(PageCreator page)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_creators.append(page);
    }

    template<typename PageClass>
    void addPage()
    {
        addPageCreator([](){return new PageClass();});
    }

private:
    QList<PageCreator> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_creators;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dialogTitle;
};
