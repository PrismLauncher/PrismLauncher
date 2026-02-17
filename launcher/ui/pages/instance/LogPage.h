// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#pragma once

#include <QIdentityProxyModel>
#include <QWidget>

#include "BaseInstance.h"
#include "launch/LaunchTask.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class LogPage;
}
class QTextCharFormat;

class LogFormatProxyModel : public QIdentityProxyModel {
   public:
    LogFormatProxyModel(QObject* parent = nullptr) : QIdentityProxyModel(parent) {}
    QVariant data(const QModelIndex& index, int role) const override;
    QFont getFont() const { return m_font; }
    void setFont(QFont font) { m_font = font; }
    QModelIndex find(const QModelIndex& start, const QString& value, bool reverse) const;

   private:
    QFont m_font;
};

class LogPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit LogPage(BaseInstance* instance, QWidget* parent = 0);
    virtual ~LogPage();
    virtual QString displayName() const override { return tr("Minecraft Log"); }
    virtual QIcon icon() const override { return QIcon::fromTheme("log"); }
    virtual QString id() const override { return "console"; }
    virtual bool apply() override;
    virtual QString helpPage() const override { return "Minecraft-Logs"; }
    virtual bool shouldDisplay() const override;
    void retranslate() override;

   private slots:
    void on_btnPaste_clicked();
    void on_btnCopy_clicked();
    void on_btnClear_clicked();
    void on_btnBottom_clicked();

    void on_trackLogCheckbox_clicked(bool checked);
    void on_wrapCheckbox_clicked(bool checked);
    void on_colorCheckbox_clicked(bool checked);

    void on_findButton_clicked();
    void findActivated();
    void findNextActivated();
    void findPreviousActivated();

    void onInstanceLaunchTaskChanged(LaunchTask* proc);

   private:
    void modelStateToUI();
    void UIToModelState();
    void setInstanceLaunchTaskChanged(LaunchTask* proc, bool initial);

   private:
    Ui::LogPage* ui;
    BaseInstance* m_instance;
    LaunchTask* m_process;

    LogFormatProxyModel* m_proxy;
    shared_qobject_ptr<LogModel> m_model;
};
