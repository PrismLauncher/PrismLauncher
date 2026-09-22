// SPDX-License-Identifier: GPL-3.0-only

#include "PathsPage.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "ui_PathsPage.h"

namespace {
QString currentDirectory(const QString& path)
{
    QFileInfo info(path);
    return info.isSymLink() ? info.symLinkTarget() : info.absoluteFilePath();
}
}  // namespace

PathsPage::PathsPage(MinecraftInstance* instance, QWidget* parent) : QWidget(parent), ui(new Ui::PathsPage), m_instance(instance)
{
    ui->setupUi(this);
    loadPaths();
}

PathsPage::~PathsPage()
{
    delete ui;
}

void PathsPage::openedImpl()
{
    loadPaths();
}

void PathsPage::retranslate()
{
    ui->retranslateUi(this);
}

void PathsPage::loadPaths()
{
    ui->modsDirTextBox->setText(currentDirectory(m_instance->modsRoot()));
    ui->resourcePacksDirTextBox->setText(currentDirectory(m_instance->resourcePacksDir()));
    ui->shaderPacksDirTextBox->setText(currentDirectory(m_instance->shaderPacksDir()));
    ui->worldsDirTextBox->setText(currentDirectory(m_instance->worldDir()));
    ui->screenshotsDirTextBox->setText(currentDirectory(FS::PathCombine(m_instance->gameRoot(), "screenshots")));
}

void PathsPage::browseForDirectory(QLineEdit* pathEdit, const QString& title)
{
    QString rawDir = QFileDialog::getExistingDirectory(this, title, pathEdit->text());
    if (!rawDir.isEmpty()) {
        pathEdit->setText(FS::NormalizePath(rawDir));
    }
}

void PathsPage::on_modsDirBrowseBtn_clicked()
{
    browseForDirectory(ui->modsDirTextBox, tr("Mods Folder"));
}

void PathsPage::on_resourcePacksDirBrowseBtn_clicked()
{
    browseForDirectory(ui->resourcePacksDirTextBox, tr("Resource Packs Folder"));
}

void PathsPage::on_shaderPacksDirBrowseBtn_clicked()
{
    browseForDirectory(ui->shaderPacksDirTextBox, tr("Shader Packs Folder"));
}

void PathsPage::on_worldsDirBrowseBtn_clicked()
{
    browseForDirectory(ui->worldsDirTextBox, tr("Worlds Folder"));
}

void PathsPage::on_screenshotsDirBrowseBtn_clicked()
{
    browseForDirectory(ui->screenshotsDirTextBox, tr("Screenshots Folder"));
}
