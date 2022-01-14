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

#include <QDialog>
#include <QVBoxLayout>

#include "BaseVersion.h"
#include "ui/pages/BasePageProvider.h"
#include "minecraft/mod/ModFolderModel.h"
#include "ModDownloadTask.h"

namespace Ui
{
class ModDownloadDialog;
}

class PageContainer;
class QDialogButtonBox;
class ModrinthPage;

class ModDownloadDialog : public QDialog, public BasePageProvider
{
    Q_OBJECT

public:
    explicit ModDownloadDialog(const std::shared_ptr<ModFolderModel>& mods, QWidget *parent = nullptr);
    ~ModDownloadDialog();

    QString dialogTitle() override;
    QList<BasePage *> getPages() override;

    void setSuggestedMod(const QString & name = QString(), ModDownloadTask * task = nullptr);

    ModDownloadTask * getTask();

public slots:
    void accept() override;
    void reject() override;

//private slots:

private:
    Ui::ModDownloadDialog *ui = nullptr;
    PageContainer * m_container = nullptr;
    QDialogButtonBox * m_buttons = nullptr;
    QVBoxLayout *m_verticalLayout = nullptr;


    ModrinthPage *modrinthPage = nullptr;
    std::unique_ptr<ModDownloadTask> modTask;
};
