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
#include <QModelIndex>
#include <memory>

class BaseInstance;
class PackIgnoreProxy;
typedef std::shared_ptr<BaseInstance> InstancePtr;

namespace Ui
{
class ExportInstanceDialog;
}

class ExportInstanceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportInstanceDialog(InstancePtr instance, QWidget *parent = 0);
    ~ExportInstanceDialog();

    virtual void done(int result);

private:
    bool doExport();
    void loadPackIgnore();
    void savePackIgnore();
    QString ignoreFileName();

private:
    Ui::ExportInstanceDialog *ui;
    InstancePtr m_instance;
    PackIgnoreProxy * proxyModel;

private slots:
    void rowsInserted(QModelIndex parent, int top, int bottom);
};
