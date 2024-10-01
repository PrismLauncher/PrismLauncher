// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
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

#include <QIdentityProxyModel>
#include <QScrollBar>
#include <QShortcut>

#include "launch/LaunchTask.h"
#include "settings/Setting.h"

#include "ui/GuiUtil.h"
#include "ui/themes/ThemeManager.h"

#include <BuildConfig.h>

class LogFormatProxyModel : public QIdentityProxyModel {
   public:
    LogFormatProxyModel(QObject* parent = nullptr) : QIdentityProxyModel(parent) {}
    QVariant data(const QModelIndex& index, int role) const override
    {
        const LogColors& colors = APPLICATION->themeManager()->getLogColors();

        switch (role) {
            case Qt::FontRole:
                return m_font;
            case Qt::ForegroundRole: {
                auto level = static_cast<MessageLevel::Enum>(QIdentityProxyModel::data(index, LogModel::LevelRole).toInt());
                QColor result = colors.foreground.value(level);

                if (result.isValid())
                    return result;

                break;
            }
            case Qt::BackgroundRole: {
                auto level = static_cast<MessageLevel::Enum>(QIdentityProxyModel::data(index, LogModel::LevelRole).toInt());
                QColor result = colors.background.value(level);

                if (result.isValid())
                    return result;

                break;
            }
        }

        return QIdentityProxyModel::data(index, role);
    }

    void setFont(QFont font) { m_font = font; }

    QModelIndex find(const QModelIndex& start, const QString& value, bool reverse) const
    {
        QModelIndex parentIndex = parent(start);
        auto compare = [&](int r) -> QModelIndex {
            QModelIndex idx = index(r, start.column(), parentIndex);
            if (!idx.isValid() || idx == start) {
                return QModelIndex();
            }
            QVariant v = data(idx, Qt::DisplayRole);
            QString t = v.toString();
            if (t.contains(value, Qt::CaseInsensitive))
                return idx;
            return QModelIndex();
        };
        if (reverse) {
            int from = start.row();
            int to = 0;

            for (int i = 0; i < 2; ++i) {
                for (int r = from; (r >= to); --r) {
                    auto idx = compare(r);
                    if (idx.isValid())
                        return idx;
                }
                // prepare for the next iteration
                from = rowCount() - 1;
                to = start.row();
            }
        } else {
            int from = start.row();
            int to = rowCount(parentIndex);

            for (int i = 0; i < 2; ++i) {
                for (int r = from; (r < to); ++r) {
                    auto idx = compare(r);
                    if (idx.isValid())
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
    QFont m_font;
};

LogPage::LogPage(InstancePtr instance, QWidget* parent) : QWidget(parent), ui(new Ui::LogPage), m_instance(instance)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();

    m_proxy = new LogFormatProxyModel(this);

    // set up fonts in the log proxy
    {
        QString fontFamily = APPLICATION->settings()->get("ConsoleFont").toString();
        bool conversionOk = false;
        int fontSize = APPLICATION->settings()->get("ConsoleFontSize").toInt(&conversionOk);
        if (!conversionOk) {
            fontSize = 11;
        }
        m_proxy->setFont(QFont(fontFamily, fontSize));
    }

    ui->text->setModel(m_proxy);

    // set up instance and launch process recognition
    {
        auto launchTask = m_instance->getLaunchTask();
        if (launchTask) {
            setInstanceLaunchTaskChanged(launchTask, true);
        }
        connect(m_instance.get(), &BaseInstance::launchTaskChanged, this, &LogPage::onInstanceLaunchTaskChanged);
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
    if (m_model->wrapLines()) {
        ui->text->setWordWrap(true);
        ui->wrapCheckbox->setCheckState(Qt::Checked);
    } else {
        ui->text->setWordWrap(false);
        ui->wrapCheckbox->setCheckState(Qt::Unchecked);
    }
    if (m_model->suspended()) {
        ui->trackLogCheckbox->setCheckState(Qt::Unchecked);
    } else {
        ui->trackLogCheckbox->setCheckState(Qt::Checked);
    }
}

void LogPage::UIToModelState()
{
    if (!m_model) {
        return;
    }
    m_model->setLineWrap(ui->wrapCheckbox->checkState() == Qt::Checked);
    m_model->suspend(ui->trackLogCheckbox->checkState() != Qt::Checked);
}

void LogPage::setInstanceLaunchTaskChanged(shared_qobject_ptr<LaunchTask> proc, bool initial)
{
    m_process = proc;
    if (m_process) {
        m_model = proc->getLogModel();
        m_proxy->setSourceModel(m_model.get());
        if (initial) {
            modelStateToUI();
        } else {
            UIToModelState();
        }
    } else {
        m_proxy->setSourceModel(nullptr);
        m_model.reset();
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
    return true;
}

void LogPage::on_btnPaste_clicked()
{
    if (!m_model)
        return;

    // FIXME: turn this into a proper task and move the upload logic out of GuiUtil!
    m_model->append(MessageLevel::Launcher,
                    QString("Log upload triggered at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    auto url = GuiUtil::uploadPaste(tr("Minecraft Log"), m_model->toPlainText(), this);
    if (!url.has_value()) {
        m_model->append(MessageLevel::Error, QString("Log upload canceled"));
    } else if (url->isNull()) {
        m_model->append(MessageLevel::Error, QString("Log upload failed!"));
    } else {
        m_model->append(MessageLevel::Launcher, QString("Log uploaded to: %1").arg(url.value()));
    }
}

void LogPage::on_btnCopy_clicked()
{
    if (!m_model)
        return;
    m_model->append(MessageLevel::Launcher, QString("Clipboard copy at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    GuiUtil::setClipboardText(m_model->toPlainText());
}

void LogPage::on_btnClear_clicked()
{
    if (!m_model)
        return;
    m_model->clear();
    m_container->refreshContainer();
}

void LogPage::on_btnBottom_clicked()
{
    ui->text->scrollToBottom();
}

void LogPage::on_trackLogCheckbox_clicked(bool checked)
{
    if (!m_model)
        return;
    m_model->suspend(!checked);
}

void LogPage::on_wrapCheckbox_clicked(bool checked)
{
    ui->text->setWordWrap(checked);
    if (!m_model)
        return;
    m_model->setLineWrap(checked);
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
    if (!ui->searchBar->hasFocus()) {
        ui->searchBar->setFocus();
        ui->searchBar->selectAll();
    }
}

void LogPage::retranslate()
{
    ui->retranslateUi(this);
}
