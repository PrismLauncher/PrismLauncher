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

#include <QString>
#include <QStringList>

namespace Ui {
class NewComponentDialog;
}

class NewComponentDialog : public QDialog {
    Q_OBJECT

   public:
    explicit NewComponentDialog(const QString& initialName = QString(), const QString& initialUid = QString(), QWidget* parent = 0);
    virtual ~NewComponentDialog();
    void setBlacklist(QStringList badUids);

    QString name() const;
    QString uid() const;

   private slots:
    void updateDialogState();

   private:
    Ui::NewComponentDialog* ui;

    QString originalPlaceholderText;
    QStringList uidBlacklist;
};
