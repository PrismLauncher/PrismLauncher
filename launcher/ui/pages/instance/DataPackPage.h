// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include <QVBoxLayout>
#include "ExternalResourcesPage.h"
#include "minecraft/mod/DataPackFolderModel.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

class DataPackPage : public ExternalResourcesPage {
    Q_OBJECT
   public:
    explicit DataPackPage(MinecraftInstance* instance, DataPackFolderModel* model, QWidget* parent = nullptr);

    static PageInfo page(MinecraftInstance* instance, DataPackFolderModel* model)
    {
        return {
            .id = "datapacks",
            .displayName = QT_TR_NOOP("Language"),
            .icon = QIcon::fromTheme("language"),
            .helpPage = "Data-packs",
            .constructor = [instance, model](QWidget* parent) { return new DataPackPage(instance, model, parent); },
        };
    }

   public slots:
    void updateFrame(const QModelIndex& current, const QModelIndex& previous) override;
    void downloadDataPacks();
    void downloadDialogFinished(int result);
    void updateDataPacks();
    void deleteDataPackMetadata();
    void changeDataPackVersion();

   private:
    DataPackFolderModel* m_model;
    QPointer<ResourceDownload::ResourceDownloadDialog> m_downloadDialog;
};

/**
 * Syncs DataPackPage with GlobalDataPacksPath and shows/hides based on GlobalDataPacksEnabled.
 */
class GlobalDataPackPage : public QWidget, public BasePage {
   public:
    explicit GlobalDataPackPage(MinecraftInstance* instance, QWidget* parent = nullptr);

    bool apply() override;
    void openedImpl() override;
    void closedImpl() override;

    void setParentContainer(BasePageContainer* container) override;

   private:
    void updateContent();
    QVBoxLayout* layout() { return static_cast<QVBoxLayout*>(QWidget::layout()); }

    // MinecraftInstance* m_instance;
    DataPackPage* m_underlyingPage = nullptr;
};
