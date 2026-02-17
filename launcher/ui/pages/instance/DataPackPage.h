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
    explicit DataPackPage(BaseInstance* instance, DataPackFolderModel* model, QWidget* parent = nullptr);

    QString displayName() const override { return QObject::tr("Data Packs"); }
    QIcon icon() const override { return QIcon::fromTheme("datapacks"); }
    QString id() const override { return "datapacks"; }
    QString helpPage() const override { return "Data-packs"; }
    bool shouldDisplay() const override { return true; }

   public slots:
    void updateFrame(const QModelIndex& current, const QModelIndex& previous) override;
    void downloadDataPacks();
    void downloadDialogFinished(int result);
    void updateDataPacks();
    void deleteDataPackMetadata();
    void changeDataPackVersion();

   private:
    DataPackFolderModel* m_model;
    QPointer<ResourceDownload::DataPackDownloadDialog> m_downloadDialog;
};

/**
 * Syncs DataPackPage with GlobalDataPacksPath and shows/hides based on GlobalDataPacksEnabled.
 */
class GlobalDataPackPage : public QWidget, public BasePage {
   public:
    explicit GlobalDataPackPage(MinecraftInstance* instance, QWidget* parent = nullptr);

    QString displayName() const override;
    QIcon icon() const override;
    QString id() const override { return "datapacks"; }
    QString helpPage() const override;

    bool shouldDisplay() const override;

    bool apply() override;
    void openedImpl() override;
    void closedImpl() override;

    void setParentContainer(BasePageContainer* container) override;

   private:
    void updateContent();
    QVBoxLayout* layout() { return static_cast<QVBoxLayout*>(QWidget::layout()); }

    MinecraftInstance* m_instance;
    DataPackPage* m_underlyingPage = nullptr;
};
