// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "ScreenshotsPage.h"
#include "BuildConfig.h"
#include "ui_ScreenshotsPage.h"

#include <QClipboard>
#include <QEvent>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QModelIndex>
#include <QMutableListIterator>
#include <QPainter>
#include <QRegularExpression>
#include <QSet>
#include <QStyledItemDelegate>

#include <Application.h>

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

#include "net/NetJob.h"
#include "screenshots/ImgurAlbumCreation.h"
#include "screenshots/ImgurUpload.h"
#include "tasks/SequentialTask.h"

#include <DesktopServices.h>
#include <FileSystem.h>
#include "RWStorage.h"

using SharedIconCache = RWStorage<QString, QIcon>;
using SharedIconCachePtr = std::shared_ptr<SharedIconCache>;

class ThumbnailingResult : public QObject {
    Q_OBJECT
   public slots:
    inline void emitResultsReady(const QString& path) { emit resultsReady(path); }
    inline void emitResultsFailed(const QString& path) { emit resultsFailed(path); }
   signals:
    void resultsReady(const QString& path);
    void resultsFailed(const QString& path);
};

class ThumbnailRunnable : public QRunnable {
   public:
    ThumbnailRunnable(QString path, SharedIconCachePtr cache)
    {
        m_path = path;
        m_cache = cache;
    }
    void run()
    {
        QFileInfo info(m_path);
        if (info.isDir())
            return;
        if ((info.suffix().compare("png", Qt::CaseInsensitive) != 0))
            return;
        if (!m_cache->stale(m_path))
            return;
        QImage image(m_path);
        if (image.isNull()) {
            m_resultEmitter.emitResultsFailed(m_path);
            qDebug() << "Error loading screenshot: " + m_path + ". Perhaps too large?";
            return;
        }
        QImage small;
        if (image.width() > image.height())
            small = image.scaledToWidth(512).scaledToWidth(256, Qt::SmoothTransformation);
        else
            small = image.scaledToHeight(512).scaledToHeight(256, Qt::SmoothTransformation);
        QPoint offset((256 - small.width()) / 2, (256 - small.height()) / 2);
        QImage square(QSize(256, 256), QImage::Format_ARGB32);
        square.fill(Qt::transparent);

        QPainter painter(&square);
        painter.drawImage(offset, small);
        painter.end();

        QIcon icon(QPixmap::fromImage(square));
        m_cache->add(m_path, icon);
        m_resultEmitter.emitResultsReady(m_path);
    }
    QString m_path;
    SharedIconCachePtr m_cache;
    ThumbnailingResult m_resultEmitter;
};

// this is about as elegant and well written as a bag of bricks with scribbles done by insane
// asylum patients.
class FilterModel : public QIdentityProxyModel {
    Q_OBJECT
   public:
    explicit FilterModel(QObject* parent = 0) : QIdentityProxyModel(parent)
    {
        m_thumbnailingPool.setMaxThreadCount(4);
        m_thumbnailCache = std::make_shared<SharedIconCache>();
        m_thumbnailCache->add("placeholder", APPLICATION->getThemedIcon("screenshot-placeholder"));
        connect(&watcher, SIGNAL(fileChanged(QString)), SLOT(fileChanged(QString)));
    }
    virtual ~FilterModel()
    {
        m_thumbnailingPool.clear();
        if (!m_thumbnailingPool.waitForDone(500))
            qDebug() << "Thumbnail pool took longer than 500ms to finish";
    }
    virtual QVariant data(const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const
    {
        auto model = sourceModel();
        if (!model)
            return QVariant();
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            QVariant result = sourceModel()->data(mapToSource(proxyIndex), role);
            return result.toString().remove(QRegularExpression("\\.png$"));
        }
        if (role == Qt::DecorationRole) {
            QVariant result = sourceModel()->data(mapToSource(proxyIndex), QFileSystemModel::FilePathRole);
            QString filePath = result.toString();
            QIcon temp;
            if (!watched.contains(filePath)) {
                ((QFileSystemWatcher&)watcher).addPath(filePath);
                ((QSet<QString>&)watched).insert(filePath);
            }
            if (m_thumbnailCache->get(filePath, temp)) {
                return temp;
            }
            if (!m_failed.contains(filePath)) {
                ((FilterModel*)this)->thumbnailImage(filePath);
            }
            return (m_thumbnailCache->get("placeholder"));
        }
        return sourceModel()->data(mapToSource(proxyIndex), role);
    }
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole)
    {
        auto model = sourceModel();
        if (!model)
            return false;
        if (role != Qt::EditRole)
            return false;
        // FIXME: this is a workaround for a bug in QFileSystemModel, where it doesn't
        // sort after renames
        {
            ((QFileSystemModel*)model)->setNameFilterDisables(true);
            ((QFileSystemModel*)model)->setNameFilterDisables(false);
        }
        return model->setData(mapToSource(index), value.toString() + ".png", role);
    }

   private:
    void thumbnailImage(QString path)
    {
        auto runnable = new ThumbnailRunnable(path, m_thumbnailCache);
        connect(&(runnable->m_resultEmitter), SIGNAL(resultsReady(QString)), SLOT(thumbnailReady(QString)));
        connect(&(runnable->m_resultEmitter), SIGNAL(resultsFailed(QString)), SLOT(thumbnailFailed(QString)));
        ((QThreadPool&)m_thumbnailingPool).start(runnable);
    }
   private slots:
    void thumbnailReady(QString path) { emit layoutChanged(); }
    void thumbnailFailed(QString path) { m_failed.insert(path); }
    void fileChanged(QString filepath)
    {
        m_thumbnailCache->setStale(filepath);
        // reinsert the path...
        watcher.removePath(filepath);
        if (QFile::exists(filepath)) {
            watcher.addPath(filepath);
            thumbnailImage(filepath);
        }
    }

   private:
    SharedIconCachePtr m_thumbnailCache;
    QThreadPool m_thumbnailingPool;
    QSet<QString> m_failed;
    QSet<QString> watched;
    QFileSystemWatcher watcher;
};

class CenteredEditingDelegate : public QStyledItemDelegate {
   public:
    explicit CenteredEditingDelegate(QObject* parent = 0) : QStyledItemDelegate(parent) {}
    virtual ~CenteredEditingDelegate() {}
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        auto widget = QStyledItemDelegate::createEditor(parent, option, index);
        auto foo = dynamic_cast<QLineEdit*>(widget);
        if (foo) {
            foo->setAlignment(Qt::AlignHCenter);
            foo->setFrame(true);
            foo->setMaximumWidth(192);
        }
        return widget;
    }
};

ScreenshotsPage::ScreenshotsPage(QString path, QWidget* parent) : QMainWindow(parent), ui(new Ui::ScreenshotsPage)
{
    m_model.reset(new QFileSystemModel());
    m_filterModel.reset(new FilterModel());
    m_filterModel->setSourceModel(m_model.get());
    m_model->setFilter(QDir::Files);
    m_model->setReadOnly(false);
    m_model->setNameFilters({ "*.png" });
    m_model->setNameFilterDisables(false);
    // Sorts by modified date instead of creation date because that column is not available and would require subclassing, this should work
    // considering screenshots aren't modified after creation.
    constexpr int file_modified_column_index = 3;
    m_model->sort(file_modified_column_index, Qt::DescendingOrder);

    m_folder = path;
    m_valid = FS::ensureFolderPathExists(m_folder);

    ui->setupUi(this);
    ui->toolBar->insertSpacer(ui->actionView_Folder);

    ui->listView->setIconSize(QSize(128, 128));
    ui->listView->setGridSize(QSize(192, 160));
    ui->listView->setSpacing(9);
    // ui->listView->setUniformItemSizes(true);
    ui->listView->setLayoutMode(QListView::Batched);
    ui->listView->setViewMode(QListView::IconMode);
    ui->listView->setResizeMode(QListView::Adjust);
    ui->listView->installEventFilter(this);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->listView->setItemDelegate(new CenteredEditingDelegate(this));
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listView, &QListView::customContextMenuRequested, this, &ScreenshotsPage::ShowContextMenu);
    connect(ui->listView, SIGNAL(activated(QModelIndex)), SLOT(onItemActivated(QModelIndex)));
}

bool ScreenshotsPage::eventFilter(QObject* obj, QEvent* evt)
{
    if (obj != ui->listView)
        return QWidget::eventFilter(obj, evt);
    if (evt->type() != QEvent::KeyPress) {
        return QWidget::eventFilter(obj, evt);
    }
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);

    if (keyEvent->matches(QKeySequence::Copy)) {
        on_actionCopy_File_s_triggered();
        return true;
    }

    switch (keyEvent->key()) {
        case Qt::Key_Delete:
            on_actionDelete_triggered();
            return true;
        case Qt::Key_F2:
            on_actionRename_triggered();
            return true;
        default:
            break;
    }
    return QWidget::eventFilter(obj, evt);
}

void ScreenshotsPage::retranslate()
{
    ui->retranslateUi(this);
}

ScreenshotsPage::~ScreenshotsPage()
{
    delete ui;
}

void ScreenshotsPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->toolBar->createContextMenu(this, tr("Context menu"));

    if (ui->listView->selectionModel()->selectedRows().size() > 1) {
        menu->removeAction(ui->actionCopy_Image);
    }

    menu->exec(ui->listView->mapToGlobal(pos));
    delete menu;
}

QMenu* ScreenshotsPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->toolBar->toggleViewAction());
    return filteredMenu;
}

void ScreenshotsPage::onItemActivated(QModelIndex index)
{
    if (!index.isValid())
        return;
    auto info = m_model->fileInfo(index);
    DesktopServices::openPath(info);
}

void ScreenshotsPage::onCurrentSelectionChanged(const QItemSelection& selected)
{
    bool allReadable = !selected.isEmpty();
    bool allWritable = !selected.isEmpty();

    for (auto index : selected.indexes()) {
        if (!index.isValid())
            break;
        auto info = m_model->fileInfo(index);
        if (!info.isReadable())
            allReadable = false;
        if (!info.isWritable())
            allWritable = false;
    }

    ui->actionUpload->setEnabled(allReadable);
    ui->actionCopy_Image->setEnabled(allReadable);
    ui->actionCopy_File_s->setEnabled(allReadable);
    ui->actionDelete->setEnabled(allWritable);
    ui->actionRename->setEnabled(allWritable);
}

void ScreenshotsPage::on_actionView_Folder_triggered()
{
    DesktopServices::openPath(m_folder, true);
}

void ScreenshotsPage::on_actionUpload_triggered()
{
    auto selection = ui->listView->selectionModel()->selectedRows();
    if (selection.isEmpty())
        return;

    QString text;
    QUrl baseUrl(BuildConfig.IMGUR_BASE_URL);
    if (selection.size() > 1)
        text = tr("You are about to upload %1 screenshots to %2.\n"
                  "You should double-check for personal information.\n\n"
                  "Are you sure?")
                   .arg(QString::number(selection.size()), baseUrl.host());
    else
        text = tr("You are about to upload the selected screenshot to %1.\n"
                  "You should double-check for personal information.\n\n"
                  "Are you sure?")
                   .arg(baseUrl.host());

    auto response = CustomMessageBox::selectable(this, "Confirm Upload", text, QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No,
                                                 QMessageBox::No)
                        ->exec();

    if (response != QMessageBox::Yes)
        return;

    QList<ScreenShot::Ptr> uploaded;
    auto job = NetJob::Ptr(new NetJob("Screenshot Upload", APPLICATION->network()));

    ProgressDialog dialog(this);
    dialog.setSkipButton(true, tr("Abort"));

    if (selection.size() < 2) {
        auto item = selection.at(0);
        auto info = m_model->fileInfo(item);
        auto screenshot = std::make_shared<ScreenShot>(info);
        job->addNetAction(ImgurUpload::make(screenshot));

        connect(job.get(), &Task::failed, [this](QString reason) {
            CustomMessageBox::selectable(this, tr("Failed to upload screenshots!"), reason, QMessageBox::Critical)->show();
        });
        connect(job.get(), &Task::aborted, [this] {
            CustomMessageBox::selectable(this, tr("Screenshots upload aborted"), tr("The task has been aborted by the user."),
                                         QMessageBox::Information)
                ->show();
        });

        m_uploadActive = true;

        if (dialog.execWithTask(job.get()) == QDialog::Accepted) {
            auto link = screenshot->m_url;
            QClipboard* clipboard = QApplication::clipboard();
            qDebug() << "ImgurUpload link" << link;
            clipboard->setText(link);
            CustomMessageBox::selectable(
                this, tr("Upload finished"),
                tr("The <a href=\"%1\">link  to the uploaded screenshot</a> has been placed in your clipboard.").arg(link),
                QMessageBox::Information)
                ->exec();
        }

        m_uploadActive = false;
        return;
    }

    for (auto item : selection) {
        auto info = m_model->fileInfo(item);
        auto screenshot = std::make_shared<ScreenShot>(info);
        uploaded.push_back(screenshot);
        job->addNetAction(ImgurUpload::make(screenshot));
    }
    SequentialTask task;
    auto albumTask = NetJob::Ptr(new NetJob("Imgur Album Creation", APPLICATION->network()));
    auto imgurResult = std::make_shared<ImgurAlbumCreation::Result>();
    auto imgurAlbum = ImgurAlbumCreation::make(imgurResult, uploaded);
    albumTask->addNetAction(imgurAlbum);
    task.addTask(job);
    task.addTask(albumTask);

    connect(&task, &Task::failed, [this](QString reason) {
        CustomMessageBox::selectable(this, tr("Failed to upload screenshots!"), reason, QMessageBox::Critical)->show();
    });
    connect(&task, &Task::aborted, [this] {
        CustomMessageBox::selectable(this, tr("Screenshots upload aborted"), tr("The task has been aborted by the user."),
                                     QMessageBox::Information)
            ->show();
    });

    m_uploadActive = true;
    if (dialog.execWithTask(&task) == QDialog::Accepted) {
        if (imgurResult->id.isEmpty()) {
            CustomMessageBox::selectable(this, tr("Failed to upload screenshots!"), tr("Unknown error"), QMessageBox::Warning)->exec();
        } else {
            auto link = QString("https://imgur.com/a/%1").arg(imgurResult->id);
            qDebug() << "ImgurUpload link" << link;
            QClipboard* clipboard = QApplication::clipboard();
            clipboard->setText(link);
            CustomMessageBox::selectable(
                this, tr("Upload finished"),
                tr("The <a href=\"%1\">link  to the uploaded album</a> has been placed in your clipboard.").arg(link),
                QMessageBox::Information)
                ->exec();
        }
    }
    m_uploadActive = false;
}

void ScreenshotsPage::on_actionCopy_Image_triggered()
{
    auto selection = ui->listView->selectionModel()->selectedRows();
    if (selection.size() < 1) {
        return;
    }

    // You can only copy one image to the clipboard. In the case of multiple selected files, only the first one gets copied.
    auto item = selection[0];
    auto info = m_model->fileInfo(item);
    QImage image(info.absoluteFilePath());
    Q_ASSERT(!image.isNull());
    QApplication::clipboard()->setImage(image, QClipboard::Clipboard);
}

void ScreenshotsPage::on_actionCopy_File_s_triggered()
{
    auto selection = ui->listView->selectionModel()->selectedRows();
    if (selection.size() < 1) {
        // Don't do anything so we don't empty the users clipboard
        return;
    }

    QString buf = "";
    for (auto item : selection) {
        auto info = m_model->fileInfo(item);
        buf += "file:///" + info.absoluteFilePath() + "\r\n";
    }
    QMimeData* mimeData = new QMimeData();
    mimeData->setData("text/uri-list", buf.toLocal8Bit());
    QApplication::clipboard()->setMimeData(mimeData);
}

void ScreenshotsPage::on_actionDelete_triggered()
{
    auto selected = ui->listView->selectionModel()->selectedIndexes();

    int count = ui->listView->selectionModel()->selectedRows().size();
    QString text;
    if (count > 1)
        text = tr("You are about to delete %1 screenshots.\n"
                  "This may be permanent and they will be gone from the folder.\n\n"
                  "Are you sure?")
                   .arg(count);
    else
        text = tr("You are about to delete the selected screenshot.\n"
                  "This may be permanent and it will be gone from the folder.\n\n"
                  "Are you sure?")
                   .arg(count);

    auto response =
        CustomMessageBox::selectable(this, tr("Confirm Deletion"), text, QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No)->exec();

    if (response != QMessageBox::Yes)
        return;

    for (auto item : selected) {
        if (FS::trash(m_model->filePath(item)))
            continue;

        m_model->remove(item);
    }
}

void ScreenshotsPage::on_actionRename_triggered()
{
    auto selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.isEmpty())
        return;
    ui->listView->edit(selection[0]);
    // TODO: mass renaming
}

void ScreenshotsPage::openedImpl()
{
    if (!m_valid) {
        m_valid = FS::ensureFolderPathExists(m_folder);
    }
    if (m_valid) {
        QString path = QDir(m_folder).absolutePath();
        auto idx = m_model->setRootPath(path);
        if (idx.isValid()) {
            ui->listView->setModel(m_filterModel.get());
            connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                    &ScreenshotsPage::onCurrentSelectionChanged);
            onCurrentSelectionChanged(ui->listView->selectionModel()->selection());  // set initial button enable states
            ui->listView->setRootIndex(m_filterModel->mapFromSource(idx));
        } else {
            ui->listView->setModel(nullptr);
        }
    }

    auto const setting_name = QString("WideBarVisibility_%1").arg(id());
    if (!APPLICATION->settings()->contains(setting_name))
        m_wide_bar_setting = APPLICATION->settings()->registerSetting(setting_name);
    else
        m_wide_bar_setting = APPLICATION->settings()->getSetting(setting_name);

    ui->toolBar->setVisibilityState(m_wide_bar_setting->get().toByteArray());
}

void ScreenshotsPage::closedImpl()
{
    m_wide_bar_setting->set(ui->toolBar->getVisibilityState());
}

#include "ScreenshotsPage.moc"
