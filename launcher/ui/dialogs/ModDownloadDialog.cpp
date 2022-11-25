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

#include "ModDownloadDialog.h"

#include "Application.h"

#include "ui/pages/modplatform/flame/FlameResourcePages.h"
#include "ui/pages/modplatform/modrinth/ModrinthResourcePages.h"

ModDownloadDialog::ModDownloadDialog(QWidget* parent, const std::shared_ptr<ModFolderModel>& mods, BaseInstance* instance)
    : ResourceDownloadDialog(parent, mods), m_instance(instance)
{
    initializeContainer();
    connectButtons();

    restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("ModDownloadGeometry").toByteArray()));
}

void ModDownloadDialog::accept()
{
    APPLICATION->settings()->set("ModDownloadGeometry", saveGeometry().toBase64());
    QDialog::accept();
}

void ModDownloadDialog::reject()
{
    APPLICATION->settings()->set("ModDownloadGeometry", saveGeometry().toBase64());
    QDialog::reject();
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
