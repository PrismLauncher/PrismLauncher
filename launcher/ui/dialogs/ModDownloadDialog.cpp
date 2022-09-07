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

#include "ModDownloadDialog.h"

#include <BaseVersion.h>
#include <InstanceList.h>
#include <icons/IconList.h>

#include "Application.h"
#include "ReviewMessageBox.h"

#include <QDialogButtonBox>
#include <QLayout>
#include <QPushButton>
#include <QValidator>

#include "ModDownloadTask.h"
#include "ui/pages/modplatform/flame/FlameModPage.h"
#include "ui/pages/modplatform/modrinth/ModrinthModPage.h"
#include "ui/widgets/PageContainer.h"

ModDownloadDialog::ModDownloadDialog(const std::shared_ptr<ModFolderModel>& mods, QWidget* parent, BaseInstance* instance)
    : QDialog(parent), mods(mods), m_verticalLayout(new QVBoxLayout(this)), m_instance(instance)
{
    setObjectName(QStringLiteral("ModDownloadDialog"));
    m_verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    resize(std::max(0.5 * parent->width(), 400.0), std::max(0.75 * parent->height(), 400.0));

    setWindowIcon(APPLICATION->getThemedIcon("new"));
    // NOTE: m_buttons must be initialized before PageContainer, because it indirectly accesses m_buttons through setSuggestedPack! Do not
    // move this below.
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    m_container = new PageContainer(this);
    m_container->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
    m_container->layout()->setContentsMargins(0, 0, 0, 0);
    m_verticalLayout->addWidget(m_container);

    m_container->addButtons(m_buttons);

    connect(m_container, &PageContainer::selectedPageChanged, this, &ModDownloadDialog::selectedPageChanged);

    // Bonk Qt over its stupid head and make sure it understands which button is the default one...
    // See: https://stackoverflow.com/questions/24556831/qbuttonbox-set-default-button
    auto OkButton = m_buttons->button(QDialogButtonBox::Ok);
    OkButton->setEnabled(false);
    OkButton->setDefault(true);
    OkButton->setAutoDefault(true);
    OkButton->setText(tr("Review and confirm"));
    OkButton->setShortcut(tr("Ctrl+Return"));
    OkButton->setToolTip(tr("Opens a new popup to review your selected mods and confirm your selection. Shortcut: Ctrl+Return"));
    connect(OkButton, &QPushButton::clicked, this, &ModDownloadDialog::confirm);

    auto CancelButton = m_buttons->button(QDialogButtonBox::Cancel);
    CancelButton->setDefault(false);
    CancelButton->setAutoDefault(false);
    connect(CancelButton, &QPushButton::clicked, this, &ModDownloadDialog::reject);

    auto HelpButton = m_buttons->button(QDialogButtonBox::Help);
    HelpButton->setDefault(false);
    HelpButton->setAutoDefault(false);
    connect(HelpButton, &QPushButton::clicked, m_container, &PageContainer::help);

    QMetaObject::connectSlotsByName(this);
    setWindowModality(Qt::WindowModal);
    setWindowTitle(dialogTitle());

    restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("ModDownloadGeometry").toByteArray()));
}

QString ModDownloadDialog::dialogTitle()
{
    return tr("Download mods");
}

void ModDownloadDialog::reject()
{
    APPLICATION->settings()->set("ModDownloadGeometry", saveGeometry().toBase64());
    QDialog::reject();
}

void ModDownloadDialog::confirm()
{
    auto keys = modTask.keys();
    keys.sort(Qt::CaseInsensitive);

    auto confirm_dialog = ReviewMessageBox::create(this, tr("Confirm mods to download"));

    for (auto& task : keys) {
        confirm_dialog->appendMod({ task, modTask.find(task).value()->getFilename() });
    }

    if (confirm_dialog->exec()) {
        auto deselected = confirm_dialog->deselectedMods();
        for (auto name : deselected) {
            modTask.remove(name);
        }

        this->accept();
    }
}

void ModDownloadDialog::accept()
{
    APPLICATION->settings()->set("ModDownloadGeometry", saveGeometry().toBase64());
    QDialog::accept();
}

QList<BasePage*> ModDownloadDialog::getPages()
{
    QList<BasePage*> pages;

    pages.append(new ModrinthModPage(this, m_instance));
    if (APPLICATION->capabilities() & Application::SupportsFlame)
        pages.append(new FlameModPage(this, m_instance));

    return pages;
}

void ModDownloadDialog::addSelectedMod(QString name, ModDownloadTask* task)
{
    removeSelectedMod(name);
    modTask.insert(name, task);

    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!modTask.isEmpty());
}

void ModDownloadDialog::removeSelectedMod(QString name)
{
    if (modTask.contains(name))
        delete modTask.find(name).value();
    modTask.remove(name);

    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!modTask.isEmpty());
}

bool ModDownloadDialog::isModSelected(QString name, QString filename) const
{
    // FIXME: Is there a way to check for versions without checking the filename
    //        as a heuristic, other than adding such info to ModDownloadTask itself?
    auto iter = modTask.find(name);
    return iter != modTask.end() && (iter.value()->getFilename() == filename);
}

bool ModDownloadDialog::isModSelected(QString name) const
{
    auto iter = modTask.find(name);
    return iter != modTask.end();
}

const QList<ModDownloadTask*> ModDownloadDialog::getTasks()
{
    return modTask.values();
}

void ModDownloadDialog::selectedPageChanged(BasePage* previous, BasePage* selected)
{
    auto* prev_page = dynamic_cast<ModPage*>(previous);
    if (!prev_page) {
        qCritical() << "Page '" << previous->displayName() << "' in ModDownloadDialog is not a ModPage!";
        return;
    }

    auto* selected_page = dynamic_cast<ModPage*>(selected);
    if (!selected_page) {
        qCritical() << "Page '" << selected->displayName() << "' in ModDownloadDialog is not a ModPage!";
        return;
    }

    // Same effect as having a global search bar
    selected_page->setSearchTerm(prev_page->getSearchTerm());
}
