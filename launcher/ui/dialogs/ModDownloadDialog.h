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

#include "ModDownloadTask.h"
#include "minecraft/mod/ModFolderModel.h"
#include "ui/pages/BasePageProvider.h"

namespace Ui
{
class ModDownloadDialog;
}

class PageContainer;
class QDialogButtonBox;
class ModrinthModPage;

class ModDownloadDialog final : public QDialog, public BasePageProvider
{
    Q_OBJECT

public:
    explicit ModDownloadDialog(const std::shared_ptr<ModFolderModel>& mods, QWidget* parent, BaseInstance* instance);
    ~ModDownloadDialog() override = default;

    QString dialogTitle() override;
    QList<BasePage*> getPages() override;

    void addSelectedMod(QString name = QString(), ModDownloadTask* task = nullptr);
    void removeSelectedMod(QString name = QString());
    bool isModSelected(QString name, QString filename) const;
    bool isModSelected(QString name) const;

    const QList<ModDownloadTask*> getTasks();
    const std::shared_ptr<ModFolderModel> &mods;

public slots:
    void confirm();
    void accept() override;
    void reject() override;

private slots:
    void selectedPageChanged(BasePage* previous, BasePage* selected);

private:
    Ui::ModDownloadDialog *ui = nullptr;
    PageContainer * m_container = nullptr;
    QDialogButtonBox * m_buttons = nullptr;
    QVBoxLayout *m_verticalLayout = nullptr;

    QHash<QString, ModDownloadTask*> modTask;
    BaseInstance *m_instance;
};
