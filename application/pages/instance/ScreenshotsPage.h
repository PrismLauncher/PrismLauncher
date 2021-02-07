/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QMainWindow>

#include "pages/BasePage.h"
#include <MultiMC.h>

class QFileSystemModel;
class QIdentityProxyModel;
namespace Ui
{
class ScreenshotsPage;
}

struct ScreenShot;
class ScreenshotList;
class ImgurAlbumCreation;

class ScreenshotsPage : public QMainWindow, public BasePage
{
    Q_OBJECT

public:
    explicit ScreenshotsPage(QString path, QWidget *parent = 0);
    virtual ~ScreenshotsPage();

    virtual void openedImpl() override;

    enum
    {
        NothingDone = 0x42
    };

    virtual bool eventFilter(QObject *, QEvent *) override;
    virtual QString displayName() const override
    {
        return tr("Screenshots");
    }
    virtual QIcon icon() const override
    {
        return MMC->getThemedIcon("screenshots");
    }
    virtual QString id() const override
    {
        return "screenshots";
    }
    virtual QString helpPage() const override
    {
        return "Screenshots-management";
    }
    virtual bool apply() override
    {
        return !m_uploadActive;
    }

protected:
    QMenu * createPopupMenu() override;

private slots:
    void on_actionUpload_triggered();
    void on_actionDelete_triggered();
    void on_actionRename_triggered();
    void on_actionView_Folder_triggered();
    void onItemActivated(QModelIndex);
    void ShowContextMenu(const QPoint &pos);

private:
    Ui::ScreenshotsPage *ui;
    std::shared_ptr<QFileSystemModel> m_model;
    std::shared_ptr<QIdentityProxyModel> m_filterModel;
    QString m_folder;
    bool m_valid = false;
    bool m_uploadActive = false;
};
