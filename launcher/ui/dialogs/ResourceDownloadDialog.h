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

    void addResource(ModPlatform::IndexedPack&, ModPlatform::IndexedVersion&, bool is_indexed = false);
    void removeResource(ModPlatform::IndexedPack&, ModPlatform::IndexedVersion&);

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

   protected:
    const std::shared_ptr<ResourceFolderModel> m_base_model;

    PageContainer* m_container = nullptr;
    ResourcePage* m_selectedPage = nullptr;

    QDialogButtonBox m_buttons;
    QVBoxLayout m_vertical_layout;

    QHash<QString, DownloadTaskPtr> m_selected;
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

}  // namespace ResourceDownload
