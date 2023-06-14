// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

    ResourceDownloadDialog(QWidget* parent, const std::shared_ptr<ResourceFolderModel> base_model);

    void initializeContainer();
    void connectButtons();

    //: String that gets appended to the download dialog title ("Download " + resourcesString())
    [[nodiscard]] virtual QString resourcesString() const { return tr("resources"); }

    QString dialogTitle() override { return tr("Download %1").arg(resourcesString()); };

    bool selectPage(QString pageId);
    ResourcePage* getSelectedPage();

    void addResource(ModPlatform::IndexedPack::Ptr, ModPlatform::IndexedVersion&);
    void removeResource(const QString&);

    const QList<DownloadTaskPtr> getTasks();
    [[nodiscard]] const std::shared_ptr<ResourceFolderModel> getBaseModel() const { return m_base_model; }

   public slots:
    void accept() override;
    void reject() override;

   protected slots:
    void selectedPageChanged(BasePage* previous, BasePage* selected);

    virtual void confirm();

   protected:
    [[nodiscard]] virtual QString geometrySaveKey() const { return ""; }
    void setButtonStatus();

   protected:
    const std::shared_ptr<ResourceFolderModel> m_base_model;

    PageContainer* m_container = nullptr;
    ResourcePage* m_selectedPage = nullptr;

    QDialogButtonBox m_buttons;
    QVBoxLayout m_vertical_layout;
};

class ModDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit ModDownloadDialog(QWidget* parent, const std::shared_ptr<ModFolderModel>& mods, BaseInstance* instance);
    ~ModDownloadDialog() override = default;

    //: String that gets appended to the mod download dialog title ("Download " + resourcesString())
    [[nodiscard]] QString resourcesString() const override { return tr("mods"); }
    [[nodiscard]] QString geometrySaveKey() const override { return "ModDownloadGeometry"; }

    QList<BasePage*> getPages() override;

   private:
    BaseInstance* m_instance;
};

class ResourcePackDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit ResourcePackDownloadDialog(QWidget* parent,
                                        const std::shared_ptr<ResourcePackFolderModel>& resource_packs,
                                        BaseInstance* instance);
    ~ResourcePackDownloadDialog() override = default;

    //: String that gets appended to the resource pack download dialog title ("Download " + resourcesString())
    [[nodiscard]] QString resourcesString() const override { return tr("resource packs"); }
    [[nodiscard]] QString geometrySaveKey() const override { return "RPDownloadGeometry"; }

    QList<BasePage*> getPages() override;

   private:
    BaseInstance* m_instance;
};

class TexturePackDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit TexturePackDownloadDialog(QWidget* parent,
                                       const std::shared_ptr<TexturePackFolderModel>& resource_packs,
                                       BaseInstance* instance);
    ~TexturePackDownloadDialog() override = default;

    //: String that gets appended to the texture pack download dialog title ("Download " + resourcesString())
    [[nodiscard]] QString resourcesString() const override { return tr("texture packs"); }
    [[nodiscard]] QString geometrySaveKey() const override { return "TPDownloadGeometry"; }

    QList<BasePage*> getPages() override;

   private:
    BaseInstance* m_instance;
};

class ShaderPackDownloadDialog final : public ResourceDownloadDialog {
    Q_OBJECT

   public:
    explicit ShaderPackDownloadDialog(QWidget* parent, const std::shared_ptr<ShaderPackFolderModel>& shader_packs, BaseInstance* instance);
    ~ShaderPackDownloadDialog() override = default;

    //: String that gets appended to the shader pack download dialog title ("Download " + resourcesString())
    [[nodiscard]] QString resourcesString() const override { return tr("shader packs"); }
    [[nodiscard]] QString geometrySaveKey() const override { return "ShaderDownloadGeometry"; }

    QList<BasePage*> getPages() override;

   private:
    BaseInstance* m_instance;
};

}  // namespace ResourceDownload
