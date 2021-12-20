/*
 * Copyright 2021 Jamie Mansfield <jmansfield@cadixdev.org>
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
#include <QAbstractListModel>

#include "modplatform/atlauncher/ATLPackIndex.h"

namespace Ui {
class AtlOptionalModDialog;
}

class AtlOptionalModListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Columns
    {
        EnabledColumn = 0,
        NameColumn,
        DescriptionColumn,
    };

    AtlOptionalModListModel(QWidget *parent, QVector<ATLauncher::VersionMod> mods);

    QVector<QString> getResult();

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
    void selectRecommended();
    void clearAll();

private:
    void toggleMod(ATLauncher::VersionMod mod, int index);
    void setMod(ATLauncher::VersionMod mod, int index, bool enable, bool shouldEmit = true);

private:
    QVector<ATLauncher::VersionMod> m_mods;
    QMap<QString, bool> m_selection;
    QMap<QString, int> m_index;
    QMap<QString, QVector<QString>> m_dependants;
};

class AtlOptionalModDialog : public QDialog {
    Q_OBJECT

public:
    AtlOptionalModDialog(QWidget *parent, QVector<ATLauncher::VersionMod> mods);
    ~AtlOptionalModDialog() override;

    QVector<QString> getResult() {
        return listModel->getResult();
    }

private:
    Ui::AtlOptionalModDialog *ui;

    AtlOptionalModListModel *listModel;
};
