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

#include <QWidget>

class QVBoxLayout;
class QTreeView;
class QLabel;
class Setting;
class QCheckBox;

class LanguageSelectionWidget : public QWidget {
    Q_OBJECT
   public:
    explicit LanguageSelectionWidget(QWidget* parent = 0);
    virtual ~LanguageSelectionWidget() {};

    QString getSelectedLanguageKey() const;
    void retranslate();

   protected slots:
    void languageRowChanged(const QModelIndex& current, const QModelIndex& previous);
    void languageSettingChanged(const Setting&, const QVariant&);

   private:
    QVBoxLayout* verticalLayout = nullptr;
    QTreeView* languageView = nullptr;
    QLabel* helpUsLabel = nullptr;
    QCheckBox* formatCheckbox = nullptr;
};
