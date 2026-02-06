// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QDialog>
#include <QDialogButtonBox>
#include <QHash>
#include <QLayout>

#include "QObjectPtr.h"
#include "minecraft/mod/tasks/GetModDependenciesTask.h"
#include "modplatform/ModIndex.h"
#include "ui/pages/BasePageProvider.h"

class BaseInstance;
class ModFolderModel;
class PageContainer;
class QVBoxLayout;
class QDialogButtonBox;
class ResourceDownloadTask;
class ResourceFolderModel;
class ResourcePackFolderModel;
class TexturePackFolderModel;
class ShaderPackFolderModel;

namespace ResourceDownload {

class ResourcePage;

class ResourceDownloadDialog : public QDialog, public BasePageProvider {
    Q_OBJECT

   public:
    using DownloadTaskPtr = shared_qobject_ptr<ResourceDownloadTask>;

    static ResourceDownloadDialog* createMod(QWidget* parent,
                                             ResourceFolderModel* mods,
                                             BaseInstance* instance,
                                             bool suppressInitialSearch = false);
    static ResourceDownloadDialog* createResourcePack(QWidget* parent,
                                                      ResourceFolderModel* mods,
                                                      BaseInstance* instance,
                                                      bool suppressInitialSearch = false);
    static ResourceDownloadDialog* createTexturePack(QWidget* parent,
                                                     ResourceFolderModel* mods,
                                                     BaseInstance* instance,
                                                     bool suppressInitialSearch = false);
    static ResourceDownloadDialog* createShaderPack(QWidget* parent,
                                                    ResourceFolderModel* mods,
                                                    BaseInstance* instance,
                                                    bool suppressInitialSearch = false);
    static ResourceDownloadDialog* createDataPack(QWidget* parent,
                                                  ResourceFolderModel* mods,
                                                  BaseInstance* instance,
                                                  bool suppressInitialSearch = false);

    void initializeContainer();
    void connectButtons();

    //: String that gets appended to the download dialog title ("Download " + resourcesString())
    QString resourcesString() const { return m_resourcesString; }

    QString dialogTitle() override { return tr("Download %1").arg(resourcesString()); };

    bool selectPage(QString pageId);
    ResourcePage* selectedPage();

    void addResource(ModPlatform::IndexedPack::Ptr, ModPlatform::IndexedVersion&, QString downloadReason = "standalone", QString dependentOn = "");
    void removeResource(const QString&);

    QList<DownloadTaskPtr> getTasks();
    ResourceFolderModel* getBaseModel() const { return m_base_model; }

    void setResourceMetadata(const std::shared_ptr<Metadata::ModStruct>& meta);

    QList<BasePage*> getPages() override { return m_pages; };

   public slots:
    void accept() override;
    void reject() override;

   protected slots:
    void selectedPageChanged(BasePage* previous, BasePage* selected);

    virtual void confirm();

   protected:
    ResourceDownloadDialog(QWidget* parent,
                           ResourceFolderModel* baseModel,
                           BaseInstance* instance,
                           QString resourcesString = tr("resources"),
                           QString geometrySaveKey = "",
                           bool suppressInitialSearch = false);

    QString geometrySaveKey() const { return m_geometrySaveKey; }
    void setButtonStatus();

    GetModDependenciesTask::Ptr getModDependenciesTask();

    void initPages(QList<BasePage*> pages);

   protected:
    ResourceFolderModel* m_base_model;

    PageContainer* m_container = nullptr;

    QDialogButtonBox m_buttons;
    QVBoxLayout m_vertical_layout;

   protected:
    bool m_suppressInitialSearch = false;
    BaseInstance* m_instance;

    QString m_resourcesString;
    QString m_geometrySaveKey;
    QList<BasePage*> m_pages;
};

}  // namespace ResourceDownload
