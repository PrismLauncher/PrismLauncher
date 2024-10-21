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
#include <QItemSelection>
#include <QLineEdit>
#include <QSortFilterProxyModel>

namespace Ui {
class IconPickerDialog;
}

class IconPickerDialog : public QDialog {
    Q_OBJECT

   public:
    explicit IconPickerDialog(QWidget* parent = 0);
    ~IconPickerDialog();
    int execWithSelection(QString selection);
    QString selectedIconKey;

   protected:
    virtual bool eventFilter(QObject*, QEvent*);

   private:
    Ui::IconPickerDialog* ui;
    QPushButton* buttonRemove;
    QLineEdit* searchBar;
    QSortFilterProxyModel* proxyModel;

   private slots:
    void selectionChanged(QItemSelection, QItemSelection);
    void activated(QModelIndex);
    void delayed_scroll(QModelIndex);
    void addNewIcon();
    void removeSelectedIcon();
    void openFolder();
    void filterIcons(const QString& text);
};
