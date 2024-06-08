// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
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
 */
#pragma once

#include <QMainWindow>
#include <QSortFilterProxyModel>

#include "plugin/PluginList.h"

#include <Application.h>
#include "ui/pages/BasePage.h"

namespace Ui {
class PluginPage;
}

class PluginPage : public QMainWindow, public BasePage {
    Q_OBJECT

   public:
    explicit PluginPage(QWidget* parent = 0);
    ~PluginPage();

    QString displayName() const override { return tr("Plugins"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("plugins"); }
    QString id() const override { return "plugins"; }
    QString helpPage() const override { return "Launcher-plugins"; }

    virtual bool shouldDisplay() const override { return true; }

    void saveColumns();
    void loadColumns();

   public slots:
    bool current(const QModelIndex& current, const QModelIndex& previous);

    virtual bool onSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

   protected slots:
    void itemActivated(const QModelIndex& index);
    void filterTextChanged(const QString& newContents);

    void removeItem();

    virtual void enableItem();
    virtual void disableItem();

    virtual void viewFolder();

    void ShowContextMenu(const QPoint& pos);
    void ShowHeaderContextMenu(const QPoint& pos);

   private:
    Ui::PluginPage* ui;
    std::shared_ptr<PluginList> m_model;
    QSortFilterProxyModel* m_filterModel = nullptr;

    QString m_fileSelectionFilter;
    QString m_viewFilter;

    std::shared_ptr<Setting> m_wide_bar_setting = nullptr;
};
