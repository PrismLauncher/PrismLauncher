// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 初夏同学 <2411829240@qq.com>
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
 */

#pragma once

#include <QMainWindow>
#include <memory>

#include "ui/pages/BasePage.h"

#include "Application.h"
class QStringListModel;

namespace Ui {
class ExternalInstancePage;
}

class ExternalInstancePage : public QMainWindow, public BasePage {
    Q_OBJECT

   public:
    inline static bool verifyInstDirPath(const QString& raw_dir);

    explicit ExternalInstancePage(QWidget* parent = 0);
    ~ExternalInstancePage();

    QString displayName() const override { return tr("External Instance"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("viewfolder"); }
    QString id() const override { return "external-Instance-directory"; }
    QString helpPage() const override { return "Getting-Started#adding-an-account"; }
    void retranslate() override;
    bool apply() override;
    void openedImpl() override;
   public slots:
    void on_actionRemove_triggered();
    void on_actionAddExtInst_triggered();
    void on_actionHide_triggered();

   protected slots:
    void ShowContextMenu(const QPoint& pos);

   private:
    void changeEvent(QEvent* event) override;
    QMenu* createPopupMenu() override;

    QString m_rootInstDir;
    QStringListModel* m_model;
    Ui::ExternalInstancePage* ui;
};
