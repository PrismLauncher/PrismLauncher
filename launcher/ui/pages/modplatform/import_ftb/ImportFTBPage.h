// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
#include <QTextBrowser>
#include <QTreeView>
#include <QWidget>

#include <Application.h>
#include "modplatform/import_ftb/PackHelpers.h"
#include "ui/pages/BasePage.h"
#include "ui/pages/modplatform/import_ftb/ListModel.h"

class NewInstanceDialog;

namespace FTBImportAPP {
namespace Ui {
class ImportFTBPage;
}

class ImportFTBPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit ImportFTBPage(NewInstanceDialog* dialog, QWidget* parent = 0);
    virtual ~ImportFTBPage();
    QString displayName() const override { return tr("FTB App Import"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("ftb_logo"); }
    QString id() const override { return "import_ftb"; }
    QString helpPage() const override { return "FTB-platform"; }
    bool shouldDisplay() const override { return true; }
    void openedImpl() override;
    void retranslate() override;

   private:
    void suggestCurrent();
    void onPackSelectionChanged(Modpack* pack = nullptr);
   private slots:
    void onSortingSelectionChanged(QString data);
    void onPublicPackSelectionChanged(QModelIndex first, QModelIndex second);
    void triggerSearch();

   private:
    bool initialized = false;
    Modpack selected;
    ListModel* listModel = nullptr;
    FilterModel* currentModel = nullptr;

    NewInstanceDialog* dialog = nullptr;
    Ui::ImportFTBPage* ui = nullptr;
};

}  // namespace FTBImportAPP
