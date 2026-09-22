// SPDX-License-Identifier: GPL-3.0-only

#include "PathsPage.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QMessageBox>
#include <filesystem>

#include "FileSystem.h"
#include "StringUtils.h"
#include "minecraft/MinecraftInstance.h"
#include "ui_PathsPage.h"

namespace {
QString currentDirectory(const QString& path)
{
    QFileInfo info(path);
    return info.isSymLink() ? info.symLinkTarget() : info.absoluteFilePath();
}

bool isInside(const QString& path, const QString& parent)
{
    const QString prefix = parent.endsWith('/') ? parent : parent + '/';
#ifdef Q_OS_WIN
    return path.compare(parent, Qt::CaseInsensitive) == 0 || path.startsWith(prefix, Qt::CaseInsensitive);
#else
    return path == parent || path.startsWith(prefix);
#endif
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

void PathsPage::browseForDirectory(const QString& path, QLineEdit* pathEdit, const QString& title)
{
    QString rawDir = QFileDialog::getExistingDirectory(this, title, pathEdit->text());
    if (!rawDir.isEmpty())
        changeDirectory(path, rawDir, pathEdit);
}

void PathsPage::changeDirectory(const QString& path, const QString& selected, QLineEdit* pathEdit)
{
    const QFileInfo original(path);
    const QString local = original.absoluteFilePath();
    const auto fsPath = std::filesystem::path(StringUtils::toStdString(local));
    const QString oldTarget = original.isSymLink() ? original.symLinkTarget() : QString();
    const QString chosen = QFileInfo(selected).absoluteFilePath();
    auto showError = [this](const QString& message) { QMessageBox::critical(this, tr("Could not change folder"), message); };

    if (original.isSymLink() && oldTarget.isEmpty()) {
        showError(tr("Could not read the existing link: %1").arg(local));
        return;
    }

    if (chosen == local && original.isDir() && !original.isSymLink()) {
        pathEdit->setText(local);
        return;
    }

    if (m_instance->isRunning()) {
        showError(tr("Stop the instance before changing its folders."));
        return;
    }

    if (chosen == local && !original.isSymLink()) {
        if (!QDir().mkpath(local))
            showError(tr("Could not create folder: %1").arg(local));
        else
            pathEdit->setText(local);
        return;
    }

    std::error_code error;
    if (chosen == local) {
        std::filesystem::remove(fsPath, error);
        if (error) {
            showError(tr("Could not remove link: %1").arg(QString::fromStdString(error.message())));
            return;
        }
        if (!QDir().mkpath(local)) {
            QString message = tr("Could not create folder: %1").arg(local);
            std::filesystem::create_directory_symlink(StringUtils::toStdString(oldTarget), fsPath, error);
            if (error)
                message += '\n' + tr("Could not restore the previous link: %1").arg(QString::fromStdString(error.message()));
            showError(message);
            return;
        }
        pathEdit->setText(local);
        return;
    }

    const QString target = QFileInfo(selected).canonicalFilePath();
    if (target.isEmpty() || !QFileInfo(target).isDir()) {
        showError(tr("Folder does not exist: %1").arg(selected));
        return;
    }

    const QString root = QFileInfo(m_instance->instanceRoot()).canonicalFilePath();
    if (!root.isEmpty() && (isInside(target, root) || isInside(root, target))) {
        showError(tr("Choose a folder outside the instance."));
        return;
    }
    if (original.isSymLink() && QFileInfo(local).canonicalFilePath() == target) {
        pathEdit->setText(target);
        return;
    }
    if (original.exists() && !original.isDir()) {
        showError(tr("Instance path is not a folder: %1").arg(local));
        return;
    }

    QString backup;
    bool wasEmpty = false;
    if (original.isSymLink() || original.exists()) {
        if (original.isSymLink() || QDir(local).entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot).isEmpty()) {
            wasEmpty = !original.isSymLink();
            std::filesystem::remove(fsPath, error);
        } else {
            const QString base = local + ".previous" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
            backup = base;
            for (int suffix = 1; QFileInfo(backup).exists() || QFileInfo(backup).isSymLink(); ++suffix)
                backup = base + QString::number(suffix);
            std::filesystem::rename(fsPath, std::filesystem::path(StringUtils::toStdString(backup)), error);
        }
        if (error) {
            showError(tr("Could not move the original folder: %1").arg(QString::fromStdString(error.message())));
            return;
        }
    }

    FS::create_link link(target, local);
    if (!link.linkRecursively(false)()) {
        QString message = tr("Could not create link: %1").arg(QString::fromStdString(link.getOSError().message()));
        if (!backup.isEmpty())
            std::filesystem::rename(std::filesystem::path(StringUtils::toStdString(backup)), fsPath, error);
        else if (!oldTarget.isEmpty())
            std::filesystem::create_directory_symlink(StringUtils::toStdString(oldTarget), fsPath, error);
        else if (wasEmpty)
            std::filesystem::create_directory(fsPath, error);

        if (error) {
            if (!backup.isEmpty())
                message += '\n' + tr("Original folder remains at %1: %2").arg(backup, QString::fromStdString(error.message()));
            else if (!oldTarget.isEmpty())
                message += '\n' + tr("Could not restore the previous link: %1").arg(QString::fromStdString(error.message()));
            else
                message += '\n' + tr("Could not restore the original folder: %1").arg(QString::fromStdString(error.message()));
        }
        showError(message);
        return;
    }
    pathEdit->setText(target);
}

void PathsPage::on_modsDirBrowseBtn_clicked()
{
    browseForDirectory(m_instance->modsRoot(), ui->modsDirTextBox, tr("Mods Folder"));
}

void PathsPage::on_resourcePacksDirBrowseBtn_clicked()
{
    browseForDirectory(m_instance->resourcePacksDir(), ui->resourcePacksDirTextBox, tr("Resource Packs Folder"));
}

void PathsPage::on_shaderPacksDirBrowseBtn_clicked()
{
    browseForDirectory(m_instance->shaderPacksDir(), ui->shaderPacksDirTextBox, tr("Shader Packs Folder"));
}

void PathsPage::on_worldsDirBrowseBtn_clicked()
{
    browseForDirectory(m_instance->worldDir(), ui->worldsDirTextBox, tr("Worlds Folder"));
}

void PathsPage::on_screenshotsDirBrowseBtn_clicked()
{
    browseForDirectory(FS::PathCombine(m_instance->gameRoot(), "screenshots"), ui->screenshotsDirTextBox, tr("Screenshots Folder"));
}

void PathsPage::on_modsDirResetBtn_clicked()
{
    changeDirectory(m_instance->modsRoot(), m_instance->modsRoot(), ui->modsDirTextBox);
}

void PathsPage::on_resourcePacksDirResetBtn_clicked()
{
    changeDirectory(m_instance->resourcePacksDir(), m_instance->resourcePacksDir(), ui->resourcePacksDirTextBox);
}

void PathsPage::on_shaderPacksDirResetBtn_clicked()
{
    changeDirectory(m_instance->shaderPacksDir(), m_instance->shaderPacksDir(), ui->shaderPacksDirTextBox);
}

void PathsPage::on_worldsDirResetBtn_clicked()
{
    changeDirectory(m_instance->worldDir(), m_instance->worldDir(), ui->worldsDirTextBox);
}

void PathsPage::on_screenshotsDirResetBtn_clicked()
{
    const QString path = FS::PathCombine(m_instance->gameRoot(), "screenshots");
    changeDirectory(path, path, ui->screenshotsDirTextBox);
}