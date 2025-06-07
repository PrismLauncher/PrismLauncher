// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "GameOptionsPage.h"
#include <QCheckBox>
#include <QComboBox>
#include <QHash>
#include <QLineEdit>
#include <QSpinBox>
#include "minecraft/MinecraftInstance.h"
#include "minecraft/gameoptions/GameOptions.h"
#include "ui_GameOptionsPage.h"

void setupWidgetsForView(QTreeView* view, QAbstractItemModel* model)
{
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex typeIndex = model->index(row, 0);
        QModelIndex valueIndex = model->index(row, 1);
        QString key = model->data(typeIndex).toString();
        auto value = model->data(valueIndex);
        auto min = model->data(model->index(row, 0), Qt::UserRole + 1);
        auto max = model->data(model->index(row, 1), Qt::UserRole + 1);
        auto values = model->data(model->index(row, 2), Qt::UserRole + 1);

        if (values.isValid() && !values.isNull() && values.canConvert<QVariantList>()) {
            auto items = values.value<QVariantList>();
            if (!items.isEmpty()) {
                QComboBox* combo = new QComboBox(view);
                int curIdx = 0;
                int i = 0;
                for (auto v : items) {
                    auto data = v.value<QVariantList>();
                    combo->addItem(data.at(0).toString(), data.at(1));
                    if (data.at(1) == value) {
                        curIdx = i;
                    }
                    i++;
                }
                combo->setCurrentIndex(curIdx);
                QObject::connect(combo, &QComboBox::currentIndexChanged, view,
                                 [model, valueIndex, combo](int index) { model->setData(valueIndex, combo->itemData(index)); });
                view->setIndexWidget(valueIndex, combo);
                continue;
            }
        }
        if (min.isValid() && max.isValid()) {
            if (min.typeId() == QMetaType::Int) {
                QSpinBox* sb = new QSpinBox(view);
                if (min != max)
                    sb->setRange(min.toInt(), max.toInt());
                auto val = value.toInt();
                sb->setValue(val);
                QObject::connect(sb, qOverload<int>(&QSpinBox::valueChanged),
                                 [model, valueIndex](int v) { model->setData(valueIndex, v, Qt::EditRole); });
                view->setIndexWidget(valueIndex, sb);
                continue;
            } else if (min.typeId() == QMetaType::Float || min.typeId() == QMetaType::Double) {
                QDoubleSpinBox* dsb = new QDoubleSpinBox(view);
                if (min != max)
                    dsb->setRange(min.toDouble(), max.toDouble());
                dsb->setDecimals(2);
                auto val = value.toDouble();
                dsb->setValue(val);
                QObject::connect(dsb, qOverload<double>(&QDoubleSpinBox::valueChanged),
                                 [model, valueIndex](double v) { model->setData(valueIndex, v, Qt::EditRole); });
                view->setIndexWidget(valueIndex, dsb);
                continue;
            }
        }
        if (value.typeId() == QMetaType::Bool) {
            QCheckBox* cb = new QCheckBox(view);
            cb->setAutoFillBackground(true);
            cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            bool val = model->data(valueIndex, Qt::EditRole).toBool();
            cb->setChecked(val);
            QObject::connect(cb, &QCheckBox::toggled,
                             [model, valueIndex](bool checked) { model->setData(valueIndex, checked, Qt::EditRole); });
            view->setIndexWidget(valueIndex, cb);
            continue;
        }

        // fallback: simple line edit widget
        QWidget* editor = new QLineEdit(view);
        QLineEdit* le = qobject_cast<QLineEdit*>(editor);
        le->setText(model->data(valueIndex).toString());
        QObject::connect(le, &QLineEdit::textChanged, [model, valueIndex](const QString& newText) { model->setData(valueIndex, newText); });
        view->setIndexWidget(valueIndex, editor);
        continue;
    }
}

GameOptionsPage::GameOptionsPage(MinecraftInstance* inst, QWidget* parent) : QWidget(parent), ui(new Ui::GameOptionsPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    m_model = inst->gameOptionsModel();
    ui->optionsView->setModel(m_model.get());
    setupWidgetsForView(ui->optionsView, m_model.get());
    auto head = ui->optionsView->header();
    if (head->count()) {
        head->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        for (int i = 1; i < head->count(); i++) {
            head->setSectionResizeMode(i, QHeaderView::Stretch);
        }
    }
}

GameOptionsPage::~GameOptionsPage()
{
    m_model->save();
}

void GameOptionsPage::openedImpl()
{
    // m_model->observe();
}

void GameOptionsPage::closedImpl()
{
    // m_model->unobserve();
}

void GameOptionsPage::retranslate()
{
    ui->retranslateUi(this);
}