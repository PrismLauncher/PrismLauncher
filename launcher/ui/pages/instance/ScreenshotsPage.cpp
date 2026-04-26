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
#include <QMenu>
#include <QModelIndex>
#include <QMutableListIterator>
#include <QPainter>
#include <QRegularExpression>
#include <QSet>
#include <QStyledItemDelegate>
#include <memory>
#include <utility>

#include <Application.h>
#include "settings/SettingsObject.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

#include "net/NetJob.h"
#include "screenshots/ImgurAlbumCreation.h"
#include "screenshots/ImgurUpload.h"
#include "tasks/SequentialTask.h"

#include <DesktopServices.h>
#include <FileSystem.h>
#include "RWStorage.h"

class ScreenshotsFSModel : public QFileSystemModel {
   public:
    bool canDropMimeData(const QMimeData* data,
                         const Qt::DropAction action,
                         const int row,
                         const int column,
                         const QModelIndex& parent) const override
    {
        const QUrl root = QUrl::fromLocalFile(rootPath());
        // this disables reordering items inside the model
        // by rejecting drops if the file is already inside the folder
        if (data->hasUrls()) {
            for (auto& url : data->urls()) {
                if (root.isParentOf(url)) {
                    return false;
                }
            }
        }
        return QFileSystemModel::canDropMimeData(data, action, row, column, parent);
    }
};

using SharedIconCache = RWStorage<QString, QIcon>;
using SharedIconCachePtr = std::shared_ptr<SharedIconCache>;

class ThumbnailingResult : public QObject {
    Q_OBJECT
   public slots:
    void emitResultsReady(const QString& path) { emit resultsReady(path); }
    void emitResultsFailed(const QString& path) { emit resultsFailed(path); }
   signals:
    void resultsReady(const QString& path);
    void resultsFailed(const QString& path);
};

class ThumbnailRunnable : public QRunnable {
   public:
    ThumbnailRunnable(QString path, SharedIconCachePtr cache) : m_path(std::move(path)), m_cache(std::move(cache)) {}
    void run() override
    {
        const QFileInfo info(m_path);
        if (info.isDir()) {
            return;
        }
        if (info.suffix().compare("png", Qt::CaseInsensitive) != 0) {
            return;
        }
        if (!m_cache->stale(m_path)) {
            return;
        }
        const QImage image(m_path);
        if (image.isNull()) {
            m_resultEmitter.emitResultsFailed(m_path);
            qDebug() << "Error loading screenshot (perhaps too large?):" + m_path;
            return;
        }
        QImage small;
        if (image.width() > image.height()) {
            small = image.scaledToWidth(512).scaledToWidth(256, Qt::SmoothTransformation);
        } else {
            small = image.scaledToHeight(512).scaledToHeight(256, Qt::SmoothTransformation);
        }
        const QPoint offset((256 - small.width()) / 2, (256 - small.height()) / 2);
        QImage square(QSize(256, 256), QImage::Format_ARGB32);
        square.fill(Qt::transparent);

        QPainter painter(&square);
        painter.drawImage(offset, small);
        painter.end();

        const QIcon icon(QPixmap::fromImage(square));
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
    explicit FilterModel(QObject* parent = nullptr) : QIdentityProxyModel(parent)
    {
        m_thumbnailingPool.setMaxThreadCount(4);
        m_thumbnailCache = std::make_shared<SharedIconCache>();
        m_thumbnailCache->add("placeholder", QIcon::fromTheme("screenshot-placeholder"));
        connect(&watcher, &QFileSystemWatcher::fileChanged, this, &FilterModel::fileChanged);
    }
    ~FilterModel() override
    {
        m_thumbnailingPool.clear();
        if (!m_thumbnailingPool.waitForDone(500)) {
            qDebug() << "Thumbnail pool took longer than 500ms to finish";
        }
    }
    QVariant data(const QModelIndex& proxyIndex, const int role = Qt::DisplayRole) const override  // NOLINT(*-default-arguments)
    {
        const auto* model = sourceModel();
        if (!model) {
            return {};
        }
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            const QVariant result = model->data(mapToSource(proxyIndex), role);
            static const QRegularExpression s_removeChars("\\.png$");
            return result.toString().remove(s_removeChars);
        }
        if (role == Qt::DecorationRole) {
            const QVariant result = model->data(mapToSource(proxyIndex), QFileSystemModel::FilePathRole);
            const QString filePath = result.toString();
            if (!watched.contains(filePath)) {
                const_cast<QFileSystemWatcher&>(watcher).addPath(filePath);
                const_cast<QSet<QString>&>(watched).insert(filePath);
            }
            if (QIcon temp; m_thumbnailCache->get(filePath, temp)) {
                return temp;
            }
            if (!m_failed.contains(filePath)) {
                const_cast<FilterModel*>(this)->thumbnailImage(filePath);
            }
            return (m_thumbnailCache->get("placeholder"));
        }
        return model->data(mapToSource(proxyIndex), role);
    }
    bool setData(const QModelIndex& index, const QVariant& value, const int role = Qt::EditRole) override  // NOLINT(*-default-arguments)
    {
        auto* model = sourceModel();
        if (!model) {
            return false;
        }
        if (role != Qt::EditRole) {
            return false;
        }
        // FIXME: this is a workaround for a bug in QFileSystemModel, where it doesn't
        // sort after renames
        {
            static_cast<QFileSystemModel*>(model)->setNameFilterDisables(true);
            static_cast<QFileSystemModel*>(model)->setNameFilterDisables(false);
        }
        return model->setData(mapToSource(index), value.toString() + ".png", role);
    }

   private:
    void thumbnailImage(QString path)
    {
        auto* runnable = new ThumbnailRunnable(std::move(path), m_thumbnailCache);
        connect(&runnable->m_resultEmitter, &ThumbnailingResult::resultsReady, this, &FilterModel::thumbnailReady);
        connect(&runnable->m_resultEmitter, &ThumbnailingResult::resultsFailed, this, &FilterModel::thumbnailFailed);
        m_thumbnailingPool.start(runnable);
    }
   private slots:
    void thumbnailReady(const QString& /*path*/) { emit layoutChanged(); }
    void thumbnailFailed(const QString& path) { m_failed.insert(path); }
    void fileChanged(const QString& filepath)
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
    explicit CenteredEditingDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    ~CenteredEditingDelegate() override = default;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        auto* widget = QStyledItemDelegate::createEditor(parent, option, index);
        if (auto* foo = dynamic_cast<QLineEdit*>(widget)) {
            foo->setAlignment(Qt::AlignHCenter);
            foo->setFrame(true);
            foo->setMaximumWidth(192);
        }
        return widget;
    }
};

ScreenshotsPage::ScreenshotsPage(QString path, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::ScreenshotsPage), m_folder(std::move(path))
{
    m_model = std::make_shared<ScreenshotsFSModel>();
    m_filterModel = std::make_shared<FilterModel>();
    m_filterModel->setSourceModel(m_model.get());
    m_model->setFilter(QDir::Files);
    m_model->setReadOnly(false);
    m_model->setNameFilters({ "*.png" });
    m_model->setNameFilterDisables(false);
    // Sorts by modified date instead of creation date because that column is not available and would require subclassing, this should work
    // considering screenshots aren't modified after creation.
    constexpr int file_modified_column_index = 3;
    m_model->sort(file_modified_column_index, Qt::DescendingOrder);

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
    connect(ui->listView, &QListView::customContextMenuRequested, this, &ScreenshotsPage::showContextMenu);
    connect(ui->listView, &QAbstractItemView::activated, this, &ScreenshotsPage::onItemActivated);
}

bool ScreenshotsPage::eventFilter(QObject* obj, QEvent* evt)
{
    if (obj != ui->listView) {
        return QWidget::eventFilter(obj, evt);
    }
    if (evt->type() != QEvent::KeyPress) {
        return QWidget::eventFilter(obj, evt);
    }
    const auto* keyEvent = static_cast<QKeyEvent*>(evt);

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

void ScreenshotsPage::showContextMenu(const QPoint& pos)
{
    auto* menu = ui->toolBar->createContextMenu(this, tr("Context menu"));

    if (ui->listView->selectionModel()->selectedIndexes().size() > 1) {
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

void ScreenshotsPage::onItemActivated(QModelIndex index) const
{
    if (!index.isValid()) {
        return;
    }
    const auto info = m_model->fileInfo(index);
    DesktopServices::openPath(info);
}

void ScreenshotsPage::onCurrentSelectionChanged(const QItemSelection& /*selected*/) const
{
    const auto selected = ui->listView->selectionModel()->selectedIndexes();

    bool allReadable = !selected.isEmpty();
    bool allWritable = !selected.isEmpty();

    for (auto index : selected) {
        if (!index.isValid()) {
            break;
        }
        auto info = m_model->fileInfo(index);
        if (!info.isReadable()) {
            allReadable = false;
        }
        if (!info.isWritable()) {
            allWritable = false;
        }
    }

    ui->actionUpload->setEnabled(allReadable);
    ui->actionCopy_Image->setEnabled(allReadable && selected.size() == 1);
    ui->actionCopy_File_s->setEnabled(allReadable);
    ui->actionDelete->setEnabled(allWritable);
    ui->actionRename->setEnabled(allWritable);
}

void ScreenshotsPage::on_actionView_Folder_triggered() const
{
    DesktopServices::openPath(m_folder, true);
}

void ScreenshotsPage::on_actionUpload_triggered()
{
    auto selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.isEmpty()) {
        return;
    }

    QString text;
    const QUrl baseUrl(BuildConfig.IMGUR_BASE_URL);
    if (selection.size() > 1) {
        text = tr("You are about to upload %1 screenshots to %2.\n"
                  "You should double-check for personal information.\n\n"
                  "Are you sure?")
                   .arg(QString::number(selection.size()), baseUrl.host());
    } else {
        text = tr("You are about to upload the selected screenshot to %1.\n"
                  "You should double-check for personal information.\n\n"
                  "Are you sure?")
                   .arg(baseUrl.host());
    }

    auto response = CustomMessageBox::selectable(this, "Confirm Upload", text, QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No,
                                                 QMessageBox::No)
                        ->exec();

    if (response != QMessageBox::Yes) {
        return;
    }

    QList<ScreenShot::Ptr> uploaded;
    auto job = NetJob::Ptr(new NetJob("Screenshot Upload", APPLICATION->network()));

    ProgressDialog dialog(this);
    dialog.setSkipButton(true, tr("Abort"));

    if (selection.size() < 2) {
        auto item = selection.at(0);
        auto info = m_model->fileInfo(item);
        auto screenshot = std::make_shared<ScreenShot>(info);
        job->addNetAction(ImgurUpload::make(screenshot));

        connect(job.get(), &Task::failed, [this](const QString& reason) {
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

    connect(&task, &Task::failed, [this](const QString& reason) {
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

void ScreenshotsPage::on_actionCopy_Image_triggered() const
{
    auto selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() < 1) {
        return;
    }

    // You can only copy one image to the clipboard. In the case of multiple selected files, only the first one gets copied.
    const auto item = selection.first();
    const auto info = m_model->fileInfo(item);
    const QImage image(info.absoluteFilePath());
    Q_ASSERT(!image.isNull());
    QApplication::clipboard()->setImage(image, QClipboard::Clipboard);
}

void ScreenshotsPage::on_actionCopy_File_s_triggered() const
{
    auto selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.size() < 1) {
        // Don't do anything so we don't empty the users clipboard
        return;
    }

    QString buf = "";
    for (auto item : selection) {
        auto info = m_model->fileInfo(item);
        buf += "file:///" + info.absoluteFilePath() + "\r\n";
    }
    auto* mimeData = new QMimeData();
    mimeData->setData("text/uri-list", buf.toLocal8Bit());
    QApplication::clipboard()->setMimeData(mimeData);
}

void ScreenshotsPage::on_actionDelete_triggered()
{
    auto selected = ui->listView->selectionModel()->selectedIndexes();

    const qsizetype count = selected.size();
    QString text;
    if (count > 1) {
        text = tr("You are about to delete %1 screenshots.\n"
                  "This may be permanent and they will be gone from the folder.\n\n"
                  "Are you sure?")
                   .arg(count);
    } else {
        text =
            tr("You are about to delete the selected screenshot.\n"
               "This may be permanent and it will be gone from the folder.\n\n"
               "Are you sure?");
    }

    const auto response =
        CustomMessageBox::selectable(this, tr("Confirm Deletion"), text, QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No)->exec();

    if (response != QMessageBox::Yes) {
        return;
    }

    for (auto item : selected) {
        if (FS::trash(m_model->filePath(item))) {
            continue;
        }

        m_model->remove(item);
    }
}

void ScreenshotsPage::on_actionRename_triggered() const
{
    auto selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.isEmpty()) {
        return;
    }
    ui->listView->edit(selection.first());
    // TODO: mass renaming
}

void ScreenshotsPage::openedImpl()
{
    if (!m_valid) {
        m_valid = FS::ensureFolderPathExists(m_folder);
    }
    if (m_valid) {
        const QString path = QDir(m_folder).absolutePath();
        const auto idx = m_model->setRootPath(path);
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

    const auto setting_name = QString("WideBarVisibility_%1").arg(id());
    m_wide_bar_setting = APPLICATION->settings()->getOrRegisterSetting(setting_name);

    ui->toolBar->setVisibilityState(QByteArray::fromBase64(m_wide_bar_setting->get().toString().toUtf8()));
}

void ScreenshotsPage::closedImpl()
{
    m_wide_bar_setting->set(QString::fromUtf8(ui->toolBar->getVisibilityState().toBase64()));
}

#include "ScreenshotsPage.moc"
