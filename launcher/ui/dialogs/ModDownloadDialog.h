// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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
 */

#pragma once

#include <QDialog>
#include <QVBoxLayout>

#include "BaseVersion.h"
#include "ui/pages/BasePageProvider.h"
#include "minecraft/mod/ModFolderModel.h"
#include "ModDownloadTask.h"
#include "ui/pages/modplatform/flame/FlameModPage.h"

namespace Ui
{
class ModDownloadDialog;
}

class PageContainer;
class QDialogButtonBox;
class ModrinthModPage;

class ModDownloadDialog : public QDialog, public BasePageProvider
{
    Q_OBJECT

public:
    explicit ModDownloadDialog(const std::shared_ptr<ModFolderModel> &mods, QWidget *parent, BaseInstance *instance);
    ~ModDownloadDialog();

    QString dialogTitle() override;
    QList<BasePage *> getPages() override;

    void addSelectedMod(const QString & name = QString(), ModDownloadTask * task = nullptr);
    void removeSelectedMod(const QString & name = QString());
    bool isModSelected(const QString & name, const QString & filename) const;
    bool isModSelected(const QString & name) const;

    const QList<ModDownloadTask*> getTasks();
    const std::shared_ptr<ModFolderModel> &mods;

public slots:
    void confirm();
    void accept() override;
    void reject() override;

private:
    Ui::ModDownloadDialog *ui = nullptr;
    PageContainer * m_container = nullptr;
    QDialogButtonBox * m_buttons = nullptr;
    QVBoxLayout *m_verticalLayout = nullptr;

    QHash<QString, ModDownloadTask*> modTask;
    BaseInstance *m_instance;
};
