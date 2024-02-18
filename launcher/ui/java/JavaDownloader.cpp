// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include "JavaDownloader.h"

#include <QPushButton>

#include <memory>

#include "Application.h"
#include "BaseVersionList.h"
#include "FileSystem.h"
#include "QObjectPtr.h"
#include "SysInfo.h"

#include "java/JavaMetadata.h"
#include "java/download/ArchiveDownloadTask.h"
#include "java/download/ManifestDownloadTask.h"

#include "meta/Index.h"
#include "meta/Version.h"

#include "meta/VersionList.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/java/VersionList.h"
#include "ui_JavaDownloader.h"

namespace Java {

Downloader::Downloader(QWidget* parent) : QDialog(parent), ui(new Ui::JavaDownloader)
{
    ui->setupUi(this);
    auto versionList = APPLICATION->metadataIndex()->get("net.minecraft.java");
    versionList->setProvidedRoles({ BaseVersionList::VersionRole, BaseVersionList::RecommendedRole, BaseVersionList::VersionPointerRole });
    ui->majorVersionSelect->initialize(versionList.get());
    ui->majorVersionSelect->selectCurrent();
    ui->majorVersionSelect->setEmptyString(tr("No java versions are currently available in the meta."));
    ui->majorVersionSelect->setEmptyErrorString(tr("Couldn't load or download the java version lists!"));

    ui->javaVersionSelect->setEmptyString(tr("No java versions are currently available for your OS."));
    ui->javaVersionSelect->setEmptyErrorString(tr("Couldn't load or download the java version lists!"));

    ui->buttonBox->button(QDialogButtonBox::Retry)->setText(tr("Refresh"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Download"));

    connect(ui->majorVersionSelect, &VersionSelectWidget::selectedVersionChanged, this, &Downloader::setSelectedVersion);
    auto reset = ui->buttonBox->button(QDialogButtonBox::Reset);
    connect(reset, &QPushButton::clicked, this, &Downloader::refresh);
}

Downloader::~Downloader()
{
    delete ui;
}

void Downloader::setSelectedVersion(BaseVersion::Ptr version)
{
    auto dcast = std::dynamic_pointer_cast<Meta::Version>(version);
    if (!dcast) {
        return;
    }
    ui->javaVersionSelect->initialize(new Java::VersionList(dcast, this));
    ui->javaVersionSelect->selectCurrent();
}

void Downloader::accept()
{
    auto meta = std::dynamic_pointer_cast<Java::Metadata>(ui->javaVersionSelect->selectedVersion());
    if (!meta) {
        return;
    }
    Task::Ptr task;
    auto final_path = FS::PathCombine(APPLICATION->javaPath(), meta->m_name);
    switch (meta->downloadType) {
        case Java::DownloadType::Manifest:
            task = makeShared<ManifestDownloadTask>(meta->url, final_path, meta->checksumType, meta->checksumHash);
            break;
        case Java::DownloadType::Archive:
            task = makeShared<ArchiveDownloadTask>(meta->url, final_path, meta->checksumType, meta->checksumHash);
            break;
    }
    auto deletePath = [final_path] { FS::deletePath(final_path); };
    connect(task.get(), &Task::failed, this, deletePath);
    connect(task.get(), &Task::aborted, this, deletePath);
    ProgressDialog pg(this);
    pg.execWithTask(task.get());
    QDialog::accept();
}

void Downloader::refresh()
{
    ui->majorVersionSelect->loadList();
}
}  // namespace Java
