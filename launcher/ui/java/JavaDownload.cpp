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

#include "JavaDownload.h"
#include <qdialogbuttonbox.h>
#include <qlogging.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <memory>
#include "Application.h"
#include "BaseVersionList.h"
#include "FileSystem.h"
#include "QObjectPtr.h"
#include "SysInfo.h"
#include "java/JavaInstallList.h"
#include "java/download/ArchiveJavaDownloader.h"
#include "java/download/ManifestJavaDownloader.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/java/ListModel.h"
#include "ui_JavaDownload.h"

JavaDownload::JavaDownload(QWidget* parent) : QDialog(parent), ui(new Ui::JavaDownload)
{
    ui->setupUi(this);
    ui->widget->initialize(new Java::JavaBaseVersionList("net.minecraft.java"));
    ui->widget->selectCurrent();
    connect(ui->widget, &VersionSelectWidget::selectedVersionChanged, this, &JavaDownload::setSelectedVersion);
    auto reset = ui->buttonBox->button(QDialogButtonBox::Reset);
    connect(reset, &QPushButton::clicked, this, &JavaDownload::refresh);
    connect(ui->widget_2, &VersionSelectWidget::selectedVersionChanged, this, &JavaDownload::setSelectedVersion2);
}

JavaDownload::~JavaDownload()
{
    delete ui;
}

void JavaDownload::setSelectedVersion(BaseVersion::Ptr version)
{
    if (!version)
        return;
    auto dcast = std::dynamic_pointer_cast<Meta::Version>(version);
    if (!dcast) {
        return;
    }
    ui->widget_2->initialize(new Java::InstallList(dcast, this));
    ui->widget_2->selectCurrent();
}

void JavaDownload::setSelectedVersion2(BaseVersion::Ptr version)
{
    if (!version)
        return;
    m_selectedVersion = std::dynamic_pointer_cast<Java::JavaRuntime2>(ui->widget_2->selectedVersion());
}
void JavaDownload::accept()
{
    if (!m_selectedVersion) {
        m_selectedVersion = std::dynamic_pointer_cast<Java::JavaRuntime2>(ui->widget_2->selectedVersion());
        qDebug() << "=========?" << (ui->widget_2->selectedVersion() != nullptr);
    }
    if (!m_selectedVersion) {
        qDebug() << "faillllllllllllllllllllllllllll";
        return;
    }
    auto meta = m_selectedVersion->meta;
    Task::Ptr task;
    auto final_path = FS::PathCombine(APPLICATION->dataRoot(), "java", meta->name);
    qDebug() << "===============>>=>>" << meta->checksumType << meta->checksumHash;
    switch (meta->downloadType) {
        case JavaRuntime::DownloadType::Manifest:
            task = makeShared<ManifestJavaDownloader>(meta->url, final_path, meta->checksumType, meta->checksumHash);
            break;
        case JavaRuntime::DownloadType::Archive:
            task = makeShared<ArchiveJavaDownloader>(meta->url, final_path, meta->checksumType, meta->checksumHash);
            break;
    }
    ProgressDialog pg(this);
    pg.execWithTask(task.get());
    QDialog::accept();
}

void JavaDownload::refresh()
{
    ui->widget->loadList();
}