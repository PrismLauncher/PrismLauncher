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

#include "ResourceDownloadDialog.h"

#include <QPushButton>

#include "Application.h"
#include "ResourceDownloadTask.h"

#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourcePackFolderModel.h"
#include "minecraft/mod/TexturePackFolderModel.h"
#include "minecraft/mod/ShaderPackFolderModel.h"

#include "ui/dialogs/ReviewMessageBox.h"

#include "ui/pages/modplatform/ResourcePage.h"

#include "ui/pages/modplatform/flame/FlameResourcePages.h"
#include "ui/pages/modplatform/modrinth/ModrinthResourcePages.h"

#include "ui/widgets/PageContainer.h"

namespace ResourceDownload {

ResourceDownloadDialog::ResourceDownloadDialog(QWidget* parent, const std::shared_ptr<ResourceFolderModel> base_model)
    : QDialog(parent), m_base_model(base_model), m_buttons(QDialogButtonBox::Help | QDialogButtonBox::Ok | QDialogButtonBox::Cancel), m_vertical_layout(this)
{
    setObjectName(QStringLiteral("ResourceDownloadDialog"));

    resize(std::max(0.5 * parent->width(), 400.0), std::max(0.75 * parent->height(), 400.0));

    setWindowIcon(APPLICATION->getThemedIcon("new"));

    // Bonk Qt over its stupid head and make sure it understands which button is the default one...
    // See: https://stackoverflow.com/questions/24556831/qbuttonbox-set-default-button
    auto OkButton = m_buttons.button(QDialogButtonBox::Ok);
    OkButton->setEnabled(false);
    OkButton->setDefault(true);
    OkButton->setAutoDefault(true);
    OkButton->setText(tr("Review and confirm"));
    OkButton->setShortcut(tr("Ctrl+Return"));

    auto CancelButton = m_buttons.button(QDialogButtonBox::Cancel);
    CancelButton->setDefault(false);
    CancelButton->setAutoDefault(false);

    auto HelpButton = m_buttons.button(QDialogButtonBox::Help);
    HelpButton->setDefault(false);
    HelpButton->setAutoDefault(false);

    setWindowModality(Qt::WindowModal);
}

void ResourceDownloadDialog::accept()
{
    if (!geometrySaveKey().isEmpty())
        APPLICATION->settings()->set(geometrySaveKey(), saveGeometry().toBase64());

    QDialog::accept();
}

void ResourceDownloadDialog::reject()
{
    if (!geometrySaveKey().isEmpty())
        APPLICATION->settings()->set(geometrySaveKey(), saveGeometry().toBase64());

    QDialog::reject();
}

// NOTE: We can't have this in the ctor because PageContainer calls a virtual function, and so
// won't work with subclasses if we put it in this ctor.
void ResourceDownloadDialog::initializeContainer()
{
    m_container = new PageContainer(this, {}, this);
    m_container->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
    m_container->layout()->setContentsMargins(0, 0, 0, 0);
    m_vertical_layout.addWidget(m_container);

    m_container->addButtons(&m_buttons);

    connect(m_container, &PageContainer::selectedPageChanged, this, &ResourceDownloadDialog::selectedPageChanged);
}

void ResourceDownloadDialog::connectButtons()
{
    auto OkButton = m_buttons.button(QDialogButtonBox::Ok);
    OkButton->setToolTip(tr("Opens a new popup to review your selected %1 and confirm your selection. Shortcut: Ctrl+Return").arg(resourcesString()));
    connect(OkButton, &QPushButton::clicked, this, &ResourceDownloadDialog::confirm);

    auto CancelButton = m_buttons.button(QDialogButtonBox::Cancel);
    connect(CancelButton, &QPushButton::clicked, this, &ResourceDownloadDialog::reject);

    auto HelpButton = m_buttons.button(QDialogButtonBox::Help);
    connect(HelpButton, &QPushButton::clicked, m_container, &PageContainer::help);
}

void ResourceDownloadDialog::confirm()
{
    auto keys = m_selected.keys();
    keys.sort(Qt::CaseInsensitive);

    auto confirm_dialog = ReviewMessageBox::create(this, tr("Confirm %1 to download").arg(resourcesString()));
    confirm_dialog->retranslateUi(resourcesString());

    for (auto& task : keys) {
        auto selected = m_selected.constFind(task).value();
        confirm_dialog->appendResource({ task, selected->getFilename(), selected->getCustomPath() });
    }

    if (confirm_dialog->exec()) {
        auto deselected = confirm_dialog->deselectedResources();
        for (auto name : deselected) {
            m_selected.remove(name);
        }

        this->accept();
    }
}

bool ResourceDownloadDialog::selectPage(QString pageId)
{
    return m_container->selectPage(pageId);
}

ResourcePage* ResourceDownloadDialog::getSelectedPage()
{
    return m_selectedPage;
}

void ResourceDownloadDialog::addResource(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, bool is_indexed)
{
    removeResource(pack, ver);

    ver.is_currently_selected = true;
    m_selected.insert(pack.name, makeShared<ResourceDownloadTask>(pack, ver, getBaseModel(), is_indexed));

    m_buttons.button(QDialogButtonBox::Ok)->setEnabled(!m_selected.isEmpty());
}

static ModPlatform::IndexedVersion& getVersionWithID(ModPlatform::IndexedPack& pack, QVariant id)
{
    Q_ASSERT(pack.versionsLoaded);
    auto it = std::find_if(pack.versions.begin(), pack.versions.end(), [id](auto const& v) { return v.fileId == id; });
    Q_ASSERT(it != pack.versions.end());
    return *it;
}

void ResourceDownloadDialog::removeResource(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver)
{
    if (auto selected_task_it = m_selected.find(pack.name); selected_task_it != m_selected.end()) {
        auto selected_task = *selected_task_it;
        auto old_version_id = selected_task->getVersionID();

        // If the new and old version IDs don't match, search for the old one and deselect it.
        if (ver.fileId != old_version_id)
            getVersionWithID(pack, old_version_id).is_currently_selected = false;
    }

    // Deselect the new version too, since all versions of that pack got removed.
    ver.is_currently_selected = false;

    m_selected.remove(pack.name);

    m_buttons.button(QDialogButtonBox::Ok)->setEnabled(!m_selected.isEmpty());
}

const QList<ResourceDownloadDialog::DownloadTaskPtr> ResourceDownloadDialog::getTasks()
{
    return m_selected.values();
}

void ResourceDownloadDialog::selectedPageChanged(BasePage* previous, BasePage* selected)
{
    auto* prev_page = dynamic_cast<ResourcePage*>(previous);
    if (!prev_page) {
        qCritical() << "Page '" << previous->displayName() << "' in ResourceDownloadDialog is not a ResourcePage!";
        return;
    }

    m_selectedPage = dynamic_cast<ResourcePage*>(selected);
    if (!m_selectedPage) {
        qCritical() << "Page '" << selected->displayName() << "' in ResourceDownloadDialog is not a ResourcePage!";
        return;
    }

    // Same effect as having a global search bar
    m_selectedPage->setSearchTerm(prev_page->getSearchTerm());
}



ModDownloadDialog::ModDownloadDialog(QWidget* parent, const std::shared_ptr<ModFolderModel>& mods, BaseInstance* instance)
    : ResourceDownloadDialog(parent, mods), m_instance(instance)
{
    setWindowTitle(dialogTitle());

    initializeContainer();
    connectButtons();

    if (!geometrySaveKey().isEmpty())
        restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get(geometrySaveKey()).toByteArray()));
}

QList<BasePage*> ModDownloadDialog::getPages()
{
    QList<BasePage*> pages;

    pages.append(ModrinthModPage::create(this, *m_instance));
    if (APPLICATION->capabilities() & Application::SupportsFlame)
        pages.append(FlameModPage::create(this, *m_instance));

    m_selectedPage = dynamic_cast<ModPage*>(pages[0]);

    return pages;
}


ResourcePackDownloadDialog::ResourcePackDownloadDialog(QWidget* parent,
                                                       const std::shared_ptr<ResourcePackFolderModel>& resource_packs,
                                                       BaseInstance* instance)
    : ResourceDownloadDialog(parent, resource_packs), m_instance(instance)
{
    setWindowTitle(dialogTitle());

    initializeContainer();
    connectButtons();

    if (!geometrySaveKey().isEmpty())
        restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get(geometrySaveKey()).toByteArray()));
}

QList<BasePage*> ResourcePackDownloadDialog::getPages()
{
    QList<BasePage*> pages;

    pages.append(ModrinthResourcePackPage::create(this, *m_instance));
    if (APPLICATION->capabilities() & Application::SupportsFlame)
        pages.append(FlameResourcePackPage::create(this, *m_instance));

    return pages;
}


TexturePackDownloadDialog::TexturePackDownloadDialog(QWidget* parent,
                                                     const std::shared_ptr<TexturePackFolderModel>& resource_packs,
                                                     BaseInstance* instance)
    : ResourceDownloadDialog(parent, resource_packs), m_instance(instance)
{
    setWindowTitle(dialogTitle());

    initializeContainer();
    connectButtons();

    if (!geometrySaveKey().isEmpty())
        restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get(geometrySaveKey()).toByteArray()));
}

QList<BasePage*> TexturePackDownloadDialog::getPages()
{
    QList<BasePage*> pages;

    pages.append(ModrinthTexturePackPage::create(this, *m_instance));
    if (APPLICATION->capabilities() & Application::SupportsFlame)
        pages.append(FlameTexturePackPage::create(this, *m_instance));

    return pages;
}


ShaderPackDownloadDialog::ShaderPackDownloadDialog(QWidget* parent,
                                                   const std::shared_ptr<ShaderPackFolderModel>& shaders,
                                                   BaseInstance* instance)
    : ResourceDownloadDialog(parent, shaders), m_instance(instance)
{
    setWindowTitle(dialogTitle());

    initializeContainer();
    connectButtons();

    if (!geometrySaveKey().isEmpty())
        restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get(geometrySaveKey()).toByteArray()));
}

QList<BasePage*> ShaderPackDownloadDialog::getPages()
{
    QList<BasePage*> pages;

    pages.append(ModrinthShaderPackPage::create(this, *m_instance));

    return pages;
}

}  // namespace ResourceDownload
