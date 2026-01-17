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
#include "minecraft/mod/DataPackFolderModel.h"
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

    ResourceDownloadDialog(QWidget* parent, ResourceFolderModel* base_model);

    void initializeContainer();
    void connectButtons();

    //: String that gets appended to the download dialog title ("Download " + resourcesString())
    virtual QString resourcesString() const { return tr("resources"); }

    QString dialogTitle() override { return tr("Download %1").arg(resourcesString()); };

    bool selectPage(QString pageId);
    ResourcePage* selectedPage();

    void addResource(ModPlatform::IndexedPack::Ptr, ModPlatform::IndexedVersion&);
    void removeResource(const QString&);

    const QList<DownloadTaskPtr> getTasks();
    ResourceFolderModel* getBaseModel() const { return m_base_model; }

    void setResourceMetadata(const std::shared_ptr<Metadata::ModStruct>& meta);

   public slots:
    void accept() override;
    void reject() override;

   protected slots:
    void selectedPageChanged(BasePage* previous, BasePage* selected);

    virtual void confirm();

   protected:
    virtual QString geometrySaveKey() const { return ""; }
    void setButtonStatus();

    virtual GetModDependenciesTask::Ptr getModDependenciesTask() { return nullptr; }

   protected:
    ResourceFolderModel* m_base_model;

    PageContainer* m_container = nullptr;

    QDialogButtonBox m_buttons;
    QVBoxLayout m_vertical_layout;
};

class ModDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit ModDownloadDialog(QWidget* parent, ModFolderModel* mods, BaseInstance* instance);
    ~ModDownloadDialog() override = default;

    //: String that gets appended to the mod download dialog title ("Download " + resourcesString())
    QString resourcesString() const override { return tr("mods"); }
    QString geometrySaveKey() const override { return "ModDownloadGeometry"; }

    QList<BasePage*> getPages() override;
    GetModDependenciesTask::Ptr getModDependenciesTask() override;

   private:
    BaseInstance* m_instance;
};

class ResourcePackDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit ResourcePackDownloadDialog(QWidget* parent, ResourcePackFolderModel* resource_packs, BaseInstance* instance);
    ~ResourcePackDownloadDialog() override = default;

    //: String that gets appended to the resource pack download dialog title ("Download " + resourcesString())
    QString resourcesString() const override { return tr("resource packs"); }
    QString geometrySaveKey() const override { return "RPDownloadGeometry"; }

    QList<BasePage*> getPages() override;

   private:
    BaseInstance* m_instance;
};

class TexturePackDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit TexturePackDownloadDialog(QWidget* parent, TexturePackFolderModel* resource_packs, BaseInstance* instance);
    ~TexturePackDownloadDialog() override = default;

    //: String that gets appended to the texture pack download dialog title ("Download " + resourcesString())
    QString resourcesString() const override { return tr("texture packs"); }
    QString geometrySaveKey() const override { return "TPDownloadGeometry"; }

    QList<BasePage*> getPages() override;

   private:
    BaseInstance* m_instance;
};

class ShaderPackDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit ShaderPackDownloadDialog(QWidget* parent, ShaderPackFolderModel* shader_packs, BaseInstance* instance);
    ~ShaderPackDownloadDialog() override = default;

    //: String that gets appended to the shader pack download dialog title ("Download " + resourcesString())
    QString resourcesString() const override { return tr("shader packs"); }
    QString geometrySaveKey() const override { return "ShaderDownloadGeometry"; }

    QList<BasePage*> getPages() override;

   private:
    BaseInstance* m_instance;
};

class DataPackDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit DataPackDownloadDialog(QWidget* parent, DataPackFolderModel* data_packs, BaseInstance* instance);
    ~DataPackDownloadDialog() override = default;

    //: String that gets appended to the data pack download dialog title ("Download " + resourcesString())
    QString resourcesString() const override { return tr("data packs"); }
    QString geometrySaveKey() const override { return "DataPackDownloadGeometry"; }

    QList<BasePage*> getPages() override;

   private:
    BaseInstance* m_instance;
};

}  // namespace ResourceDownload
