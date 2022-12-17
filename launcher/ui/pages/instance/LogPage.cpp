// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "LogPage.h"
#include "ui_LogPage.h"

#include "Application.h"

#include <QIcon>
#include <QScrollBar>
#include <QShortcut>

#include "launch/LaunchTask.h"
#include "settings/Setting.h"

#include "ui/GuiUtil.h"
#include "ui/ColorCache.h"

#include <BuildConfig.h>

class LogFormatProxyModel : public QIdentityProxyModel
{
public:
    LogFormatProxyModel(QObject* parent = nullptr) : QIdentityProxyModel(parent)
    {
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        switch(role)
        {
            case Qt::FontRole:
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_font;
            case Qt::ForegroundRole:
            {
                MessageLevel::Enum level = (MessageLevel::Enum) QIdentityProxyModel::data(index, LogModel::LevelRole).toInt();
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors->getFront(level);
            }
            case Qt::BackgroundRole:
            {
                MessageLevel::Enum level = (MessageLevel::Enum) QIdentityProxyModel::data(index, LogModel::LevelRole).toInt();
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors->getBack(level);
            }
            default:
                return QIdentityProxyModel::data(index, role);
            }
    }

    void setFont(QFont font)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_font = font;
    }

    void setColors(LogColorCache* colors)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors.reset(colors);
    }

    QModelIndex find(const QModelIndex &start, const QString &value, bool reverse) const
    {
        QModelIndex parentIndex = parent(start);
        auto compare = [&](int r) -> QModelIndex
        {
            QModelIndex idx = index(r, start.column(), parentIndex);
            if (!idx.isValid() || idx == start)
            {
                return QModelIndex();
            }
            QVariant v = data(idx, Qt::DisplayRole);
            QString t = v.toString();
            if (t.contains(value, Qt::CaseInsensitive))
                return idx;
            return QModelIndex();
        };
        if(reverse)
        {
            int from = start.row();
            int to = 0;

            for (int i = 0; i < 2; ++i)
            {
                for (int r = from; (r >= to); --r)
                {
                    auto idx = compare(r);
                    if(idx.isValid())
                        return idx;
                }
                // prepare for the next iteration
                from = rowCount() - 1;
                to = start.row();
            }
        }
        else
        {
            int from = start.row();
            int to = rowCount(parentIndex);

            for (int i = 0; i < 2; ++i)
            {
                for (int r = from; (r < to); ++r)
                {
                    auto idx = compare(r);
                    if(idx.isValid())
                        return idx;
                }
                // prepare for the next iteration
                from = 0;
                to = start.row();
            }
        }
        return QModelIndex();
    }
private:
    QFont hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_font;
    std::unique_ptr<LogColorCache> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors;
};

LogPage::LogPage(InstancePtr instance, QWidget *parent)
    : QWidget(parent), ui(new Ui::LogPage), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance(instance)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxy = new LogFormatProxyModel(this);
    // set up text colors in the log proxy and adapt them to the current theme foreground and background
    {
        auto origForeground = ui->text->palette().color(ui->text->foregroundRole());
        auto origBackground = ui->text->palette().color(ui->text->backgroundRole());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxy->setColors(new LogColorCache(origForeground, origBackground));
    }

    // set up fonts in the log proxy
    {
        QString fontFamily = APPLICATION->settings()->get("ConsoleFont").toString();
        bool conversionOk = false;
        int fontSize = APPLICATION->settings()->get("ConsoleFontSize").toInt(&conversionOk);
        if(!conversionOk)
        {
            fontSize = 11;
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxy->setFont(QFont(fontFamily, fontSize));
    }

    ui->text->setModel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxy);

    // set up instance and launch process recognition
    {
        auto launchTask = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->getLaunchTask();
        if(launchTask)
        {
            setInstanceLaunchTaskChanged(launchTask, true);
        }
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance.get(), &BaseInstance::launchTaskChanged, this, &LogPage::onInstanceLaunchTaskChanged);
    }

    auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
    connect(findShortcut, SIGNAL(activated()), SLOT(findActivated()));
    auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
    connect(findNextShortcut, SIGNAL(activated()), SLOT(findNextActivated()));
    connect(ui->searchBar, SIGNAL(returnPressed()), SLOT(on_findButton_clicked()));
    auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
    connect(findPreviousShortcut, SIGNAL(activated()), SLOT(findPreviousActivated()));
}

LogPage::~LogPage()
{
    delete ui;
}

void LogPage::modelStateToUI()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->wrapLines())
    {
        ui->text->setWordWrap(true);
        ui->wrapCheckbox->setCheckState(Qt::Checked);
    }
    else
    {
        ui->text->setWordWrap(false);
        ui->wrapCheckbox->setCheckState(Qt::Unchecked);
    }
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->suspended())
    {
        ui->trackLogCheckbox->setCheckState(Qt::Unchecked);
    }
    else
    {
        ui->trackLogCheckbox->setCheckState(Qt::Checked);
    }
}

void LogPage::UIToModelState()
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
    {
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->setLineWrap(ui->wrapCheckbox->checkState() == Qt::Checked);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->suspend(ui->trackLogCheckbox->checkState() != Qt::Checked);
}

void LogPage::setInstanceLaunchTaskChanged(shared_qobject_ptr<LaunchTask> proc, bool initial)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process = proc;
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model = proc->getLogModel();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxy->setSourceModel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model.get());
        if(initial)
        {
            modelStateToUI();
        }
        else
        {
            UIToModelState();
        }
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxy->setSourceModel(nullptr);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model.reset();
    }
}

void LogPage::onInstanceLaunchTaskChanged(shared_qobject_ptr<LaunchTask> proc)
{
    setInstanceLaunchTaskChanged(proc, false);
}

bool LogPage::apply()
{
    return true;
}

bool LogPage::shouldDisplay() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->isRunning() || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxy->rowCount() > 0;
}

void LogPage::on_btnPaste_clicked()
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
        return;

    //FIXME: turn this into a proper task and move the upload logic out of GuiUtil!
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->append(
        MessageLevel::Launcher,
        QString("%2: Log upload triggered at: %1").arg(
            QDateTime::currentDateTime().toString(Qt::RFC2822Date),
            BuildConfig.LAUNCHER_DISPLAYNAME
        )
    );
    auto url = GuiUtil::uploadPaste(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->toPlainText(), this);
    if(!url.isEmpty())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->append(
            MessageLevel::Launcher,
            QString("%2: Log uploaded to: %1").arg(
                url,
                BuildConfig.LAUNCHER_DISPLAYNAME
            )
        );
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->append(
            MessageLevel::Error,
            QString("%1: Log upload failed!").arg(BuildConfig.LAUNCHER_DISPLAYNAME)
        );
    }
}

void LogPage::on_btnCopy_clicked()
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
        return;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->append(MessageLevel::Launcher, QString("Clipboard copy at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    GuiUtil::setClipboardText(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->toPlainText());
}

void LogPage::on_btnClear_clicked()
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
        return;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->refreshContainer();
}

void LogPage::on_btnBottohello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_clicked()
{
    ui->text->scrollToBottom();
}

void LogPage::on_trackLogCheckbox_clicked(bool checked)
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
        return;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->suspend(!checked);
}

void LogPage::on_wrapCheckbox_clicked(bool checked)
{
    ui->text->setWordWrap(checked);
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
        return;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->setLineWrap(checked);
}

void LogPage::on_findButton_clicked()
{
    auto modifiers = QApplication::keyboardModifiers();
    bool reverse = modifiers & Qt::ShiftModifier;
    ui->text->findNext(ui->searchBar->text(), reverse);
}

void LogPage::findNextActivated()
{
    ui->text->findNext(ui->searchBar->text(), false);
}

void LogPage::findPreviousActivated()
{
    ui->text->findNext(ui->searchBar->text(), true);
}

void LogPage::findActivated()
{
    // focus the search bar if it doesn't have focus
    if (!ui->searchBar->hasFocus())
    {
        ui->searchBar->setFocus();
        ui->searchBar->selectAll();
    }
}

void LogPage::retranslate()
{
    ui->retranslateUi(this);
}
