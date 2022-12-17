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
    setWindowTitle(dialogTitle());
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
    m_container = new PageContainer(this);
    m_container->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
    m_container->layout()->setContentsMargins(0, 0, 0, 0);
    m_vertical_layout.addWidget(m_container);

    m_container->addButtons(&m_buttons);

    connect(m_container, &PageContainer::selectedPageChanged, this, &ResourceDownloadDialog::selectedPageChanged);
}

void ResourceDownloadDialog::connectButtons()
{
    auto OkButton = m_buttons.button(QDialogButtonBox::Ok);
    OkButton->setToolTip(tr("Opens a new popup to review your selected %1 and confirm your selection. Shortcut: Ctrl+Return").arg(resourceString()));
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

    auto confirm_dialog = ReviewMessageBox::create(this, tr("Confirm %1 to download").arg(resourceString()));

    for (auto& task : keys) {
        confirm_dialog->appendResource({ task, m_selected.find(task).value()->getFilename() });
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

void ResourceDownloadDialog::addResource(QString name, ResourceDownloadTask* task)
{
    removeResource(name);
    m_selected.insert(name, task);

    m_buttons.button(QDialogButtonBox::Ok)->setEnabled(!m_selected.isEmpty());
}

void ResourceDownloadDialog::removeResource(QString name)
{
    if (m_selected.contains(name))
        m_selected.find(name).value()->deleteLater();
    m_selected.remove(name);

    m_buttons.button(QDialogButtonBox::Ok)->setEnabled(!m_selected.isEmpty());
}

bool ResourceDownloadDialog::isSelected(QString name, QString filename) const
{
    auto iter = m_selected.constFind(name);
    if (iter == m_selected.constEnd())
        return false;

    // FIXME: Is there a way to check for versions without checking the filename
    //        as a heuristic, other than adding such info to ResourceDownloadTask itself?
    if (!filename.isEmpty())
        return iter.value()->getFilename() == filename;

    return true;
}

const QList<ResourceDownloadTask*> ResourceDownloadDialog::getTasks()
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

}  // namespace ResourceDownload
