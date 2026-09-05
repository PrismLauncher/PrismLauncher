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
#include <cstdint>

#include <QDialog>
#include <QItemSelection>
#include <QSortFilterProxyModel>

namespace Ui {
class IconPickerDialog;
}

class IconPickerDialog : public QDialog {
    Q_OBJECT

   public:
    explicit IconPickerDialog(QWidget* parent = nullptr);
    ~IconPickerDialog() override;
    int execWithSelection(const QString& selection);
    QString selectedIconKey;

    enum class IconPickerCategory : std::uint8_t {
        Any,
        Modern,
        Legacy,
        Modpacks,
        Custom,
    };
    Q_ENUM(IconPickerCategory)

   protected:
    bool eventFilter(QObject* /*unused*/, QEvent* /*unused*/) override;

   private:
    Ui::IconPickerDialog* m_ui;
    QPushButton* m_buttonRemove;
    QSortFilterProxyModel* m_proxyModel;

   private slots:
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void activated(QModelIndex);
    void delayed_scroll(QModelIndex);
    void addNewIcon();
    void removeSelectedIcon() const;
    void openFolder() const;
    void filterIcons(const QString& text);
    void filterIconsByCategory(IconPickerCategory);
};
