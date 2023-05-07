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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ExportInstanceDialog.h"
#include "ui_ExportInstanceDialog.h"
#include <BaseInstance.h>
#include <MMCZip.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileSystemModel>

#include <QSortFilterProxyModel>
#include <QDebug>
#include <QSaveFile>
#include <QStack>
#include <QFileInfo>

#include "StringUtils.h"
#include "SeparatorPrefixTree.h"
#include "Application.h"
#include <icons/IconList.h>
#include <FileSystem.h>

class PackIgnoreProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    PackIgnoreProxy(InstancePtr instance, QObject *parent) : QSortFilterProxyModel(parent)
    {
        m_instance = instance;
    }
    // NOTE: Sadly, we have to do sorting ourselves.
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const
    {
        QFileSystemModel *fsm = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!fsm)
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }
        bool asc = sortOrder() == Qt::AscendingOrder ? true : false;

        QFileInfo leftFileInfo = fsm->fileInfo(left);
        QFileInfo rightFileInfo = fsm->fileInfo(right);

        if (!leftFileInfo.isDir() && rightFileInfo.isDir())
        {
            return !asc;
        }
        if (leftFileInfo.isDir() && !rightFileInfo.isDir())
        {
            return asc;
        }

        // sort and proxy model breaks the original model...
        if (sortColumn() == 0)
        {
            return StringUtils::naturalCompare(leftFileInfo.fileName(), rightFileInfo.fileName(),
                                           Qt::CaseInsensitive) < 0;
        }
        if (sortColumn() == 1)
        {
            auto leftSize = leftFileInfo.size();
            auto rightSize = rightFileInfo.size();
            if ((leftSize == rightSize) || (leftFileInfo.isDir() && rightFileInfo.isDir()))
            {
                return StringUtils::naturalCompare(leftFileInfo.fileName(),
                                               rightFileInfo.fileName(),
                                               Qt::CaseInsensitive) < 0
                           ? asc
                           : !asc;
            }
            return leftSize < rightSize;
        }
        return QSortFilterProxyModel::lessThan(left, right);
    }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        auto sourceIndex = mapToSource(index);
        Qt::ItemFlags flags = sourceIndex.flags();
        if (index.column() == 0)
        {
            flags |= Qt::ItemIsUserCheckable;
            if (sourceIndex.model()->hasChildren(sourceIndex))
            {
                flags |= Qt::ItemIsAutoTristate;
            }
        }

        return flags;
    }

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        QModelIndex sourceIndex = mapToSource(index);

        if (index.column() == 0 && role == Qt::CheckStateRole)
        {
            QFileSystemModel *fsm = qobject_cast<QFileSystemModel *>(sourceModel());
            auto blockedPath = relPath(fsm->filePath(sourceIndex));
            auto cover = blocked.cover(blockedPath);
            if (!cover.isNull())
            {
                return QVariant(Qt::Unchecked);
            }
            else if (blocked.exists(blockedPath))
            {
                return QVariant(Qt::PartiallyChecked);
            }
            else
            {
                return QVariant(Qt::Checked);
            }
        }

        return sourceIndex.data(role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value,
                         int role = Qt::EditRole)
    {
        if (index.column() == 0 && role == Qt::CheckStateRole)
        {
            Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
            return setFilterState(index, state);
        }

        QModelIndex sourceIndex = mapToSource(index);
        return QSortFilterProxyModel::sourceModel()->setData(sourceIndex, value, role);
    }

    QString relPath(const QString &path) const
    {
        QString prefix = QDir().absoluteFilePath(m_instance->instanceRoot());
        prefix += '/';
        if (!path.startsWith(prefix))
        {
            return QString();
        }
        return path.mid(prefix.size());
    }

    bool setFilterState(QModelIndex index, Qt::CheckState state)
    {
        QFileSystemModel *fsm = qobject_cast<QFileSystemModel *>(sourceModel());

        if (!fsm)
        {
            return false;
        }

        QModelIndex sourceIndex = mapToSource(index);
        auto blockedPath = relPath(fsm->filePath(sourceIndex));
        bool changed = false;
        if (state == Qt::Unchecked)
        {
            // blocking a path
            auto &node = blocked.insert(blockedPath);
            // get rid of all blocked nodes below
            node.clear();
            changed = true;
        }
        else if (state == Qt::Checked || state == Qt::PartiallyChecked)
        {
            if (!blocked.remove(blockedPath))
            {
                auto cover = blocked.cover(blockedPath);
                qDebug() << "Blocked by cover" << cover;
                // uncover
                blocked.remove(cover);
                // block all contents, except for any cover
                QModelIndex rootIndex =
                    fsm->index(FS::PathCombine(m_instance->instanceRoot(), cover));
                QModelIndex doing = rootIndex;
                int row = 0;
                QStack<QModelIndex> todo;
                while (1)
                {
                    auto node = fsm->index(row, 0, doing);
                    if (!node.isValid())
                    {
                        if (!todo.size())
                        {
                            break;
                        }
                        else
                        {
                            doing = todo.pop();
                            row = 0;
                            continue;
                        }
                    }
                    auto relpath = relPath(fsm->filePath(node));
                    if (blockedPath.startsWith(relpath)) // cover found?
                    {
                        // continue processing cover later
                        todo.push(node);
                    }
                    else
                    {
                        // or just block this one.
                        blocked.insert(relpath);
                    }
                    row++;
                }
            }
            changed = true;
        }
        if (changed)
        {
            // update the thing
            emit dataChanged(index, index, {Qt::CheckStateRole});
            // update everything above index
            QModelIndex up = index.parent();
            while (1)
            {
                if (!up.isValid())
                    break;
                emit dataChanged(up, up, {Qt::CheckStateRole});
                up = up.parent();
            }
            // and everything below the index
            QModelIndex doing = index;
            int row = 0;
            QStack<QModelIndex> todo;
            while (1)
            {
                auto node = this->index(row, 0, doing);
                if (!node.isValid())
                {
                    if (!todo.size())
                    {
                        break;
                    }
                    else
                    {
                        doing = todo.pop();
                        row = 0;
                        continue;
                    }
                }
                emit dataChanged(node, node, {Qt::CheckStateRole});
                todo.push(node);
                row++;
            }
            // siblings and unrelated nodes are ignored
        }
        return true;
    }

    bool shouldExpand(QModelIndex index)
    {
        QModelIndex sourceIndex = mapToSource(index);
        QFileSystemModel *fsm = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!fsm)
        {
            return false;
        }
        auto blockedPath = relPath(fsm->filePath(sourceIndex));
        auto found = blocked.find(blockedPath);
        if(found)
        {
            return !found->leaf();
        }
        return false;
    }

    void setBlockedPaths(QStringList paths)
    {
        beginResetModel();
        blocked.clear();
        blocked.insert(paths);
        endResetModel();
    }

    const SeparatorPrefixTree<'/'> & blockedPaths() const
    {
        return blocked;
    }

protected:
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
    {
        Q_UNUSED(source_parent)

        // adjust the columns you want to filter out here
        // return false for those that will be hidden
        if (source_column == 2 || source_column == 3)
            return false;

        return true;
    }

private:
    InstancePtr m_instance;
    SeparatorPrefixTree<'/'> blocked;
};

ExportInstanceDialog::ExportInstanceDialog(InstancePtr instance, QWidget *parent)
    : QDialog(parent), ui(new Ui::ExportInstanceDialog), m_instance(instance)
{
    ui->setupUi(this);
    auto model = new QFileSystemModel(this);
    proxyModel = new PackIgnoreProxy(m_instance, this);
    loadPackIgnore();
    proxyModel->setSourceModel(model);
    auto root = instance->instanceRoot();
    ui->treeView->setModel(proxyModel);
    ui->treeView->setRootIndex(proxyModel->mapFromSource(model->index(root)));
    ui->treeView->sortByColumn(0, Qt::AscendingOrder);

    connect(proxyModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(rowsInserted(QModelIndex,int,int)));

    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Hidden);
    model->setRootPath(root);
    auto headerView = ui->treeView->header();
    headerView->setSectionResizeMode(QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(0, QHeaderView::Stretch);
}

ExportInstanceDialog::~ExportInstanceDialog()
{
    delete ui;
}

/// Save icon to instance's folder is needed
void SaveIcon(InstancePtr m_instance)
{
    auto iconKey = m_instance->iconKey();
    auto iconList = APPLICATION->icons();
    auto mmcIcon = iconList->icon(iconKey);
    if(!mmcIcon || mmcIcon->isBuiltIn()) {
        return;
    }
    auto path = mmcIcon->getFilePath();
    if(!path.isNull()) {
        QFileInfo inInfo (path);
        FS::copy(path, FS::PathCombine(m_instance->instanceRoot(), inInfo.fileName())) ();
        return;
    }
    auto & image = mmcIcon->m_images[mmcIcon->type()];
    auto & icon = image.icon;
    auto sizes = icon.availableSizes();
    if(sizes.size() == 0)
    {
        return;
    }
    auto areaOf = [](QSize size)
    {
        return size.width() * size.height();
    };
    QSize largest = sizes[0];
    // find variant with largest area
    for(auto size: sizes)
    {
        if(areaOf(largest) < areaOf(size))
        {
            largest = size;
        }
    }
    auto pixmap = icon.pixmap(largest);
    pixmap.save(FS::PathCombine(m_instance->instanceRoot(), iconKey + ".png"));
}

bool ExportInstanceDialog::doExport()
{
    auto name = FS::RemoveInvalidFilenameChars(m_instance->name());

    const QString output = QFileDialog::getSaveFileName(
        this, tr("Export %1").arg(m_instance->name()),
        FS::PathCombine(QDir::homePath(), name + ".zip"), "Zip (*.zip)", nullptr, QFileDialog::DontConfirmOverwrite);
    if (output.isEmpty())
    {
        return false;
    }
    if (QFile::exists(output))
    {
        int ret =
            QMessageBox::question(this, tr("Overwrite?"),
                                  tr("This file already exists. Do you want to overwrite it?"),
                                  QMessageBox::No, QMessageBox::Yes);
        if (ret == QMessageBox::No)
        {
            return false;
        }
    }

    SaveIcon(m_instance);

    auto & blocked = proxyModel->blockedPaths();
    using std::placeholders::_1;
    auto files = QFileInfoList();
    if (!MMCZip::collectFileListRecursively(m_instance->instanceRoot(), nullptr, &files,
                                    std::bind(&SeparatorPrefixTree<'/'>::covers, blocked, _1))) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to export instance"));
        return false;
    }

    if (!MMCZip::compressDirFiles(output, m_instance->instanceRoot(), files, true))
    {
        QMessageBox::warning(this, tr("Error"), tr("Unable to export instance"));
        return false;
    }
    return true;
}

void ExportInstanceDialog::done(int result)
{
    savePackIgnore();
    if (result == QDialog::Accepted)
    {
        if (doExport())
        {
            QDialog::done(QDialog::Accepted);
            return;
        }
        else
        {
            return;
        }
    }
    QDialog::done(result);
}

void ExportInstanceDialog::rowsInserted(QModelIndex parent, int top, int bottom)
{
    //WARNING: possible off-by-one?
    for(int i = top; i < bottom; i++)
    {
        auto node = proxyModel->index(i, 0, parent);
        if(proxyModel->shouldExpand(node))
        {
            auto expNode = node.parent();
            if(!expNode.isValid())
            {
                continue;
            }
            ui->treeView->expand(node);
        }
    }
}

QString ExportInstanceDialog::ignoreFileName()
{
    return FS::PathCombine(m_instance->instanceRoot(), ".packignore");
}

void ExportInstanceDialog::loadPackIgnore()
{
    auto filename = ignoreFileName();
    QFile ignoreFile(filename);
    if(!ignoreFile.open(QIODevice::ReadOnly))
    {
        return;
    }
    auto data = ignoreFile.readAll();
    auto string = QString::fromUtf8(data);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    proxyModel->setBlockedPaths(string.split('\n', Qt::SkipEmptyParts));
#else
    proxyModel->setBlockedPaths(string.split('\n', QString::SkipEmptyParts));
#endif
}

void ExportInstanceDialog::savePackIgnore()
{
    auto data = proxyModel->blockedPaths().toStringList().join('\n').toUtf8();
    auto filename = ignoreFileName();
    try
    {
        FS::write(filename, data);
    }
    catch (const Exception &e)
    {
        qWarning() << e.cause();
    }
}

#include "ExportInstanceDialog.moc"
