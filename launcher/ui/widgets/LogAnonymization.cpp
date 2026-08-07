// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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
 */

#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QTreeWidgetItem>

#include "LogAnonymization.h"
#include "ui_LogAnonymization.h"

LogAnonymization::LogAnonymization(QWidget* parent) : QWidget(parent), m_ui(new Ui::LogAnonymization)
{
    m_ui->setupUi(this);
    m_ui->list->installEventFilter(this);

    m_ui->list->setSortingEnabled(false);
    m_ui->list->header()->resizeSections(QHeaderView::Interactive);

    connect(m_ui->list, &QTreeWidget::itemChanged, this, [](QTreeWidgetItem* item, int column) {
        if (column == 0) {
            validateItem(item);
        }
    });

    connect(m_ui->add, &QPushButton::clicked, this, [this] {
        auto* item = new QTreeWidgetItem(m_ui->list);
        item->setText(0, "^\\\\S+$");
        item->setText(1, "");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_ui->list->addTopLevelItem(item);
        validateItem(item);
        m_ui->list->selectionModel()->select(m_ui->list->model()->index(m_ui->list->indexOfTopLevelItem(item), 0),
                                             QItemSelectionModel::ClearAndSelect | QItemSelectionModel::SelectionFlag::Rows);
        m_ui->list->editItem(item, 0);
    });

    connect(m_ui->remove, &QPushButton::clicked, this, [this] {
        for (QTreeWidgetItem* item : m_ui->list->selectedItems()) {
            m_ui->list->takeTopLevelItem(m_ui->list->indexOfTopLevelItem(item));
        }
    });

    connect(m_ui->clear, &QPushButton::clicked, this, [this] { m_ui->list->clear(); });
}

LogAnonymization::~LogAnonymization()
{
    delete m_ui;
}

void LogAnonymization::initialize(const QList<Rule>& rules)
{
    m_ui->list->clear();
    for (const auto& rule : rules) {
        auto* item = new QTreeWidgetItem(m_ui->list);
        item->setText(0, rule.regex);
        item->setText(1, rule.replace);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_ui->list->addTopLevelItem(item);
        validateItem(item);
    }
}

bool LogAnonymization::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_ui->list && event->type() == QEvent::KeyPress) {
        const auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            emit m_ui->remove->clicked();
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}

void LogAnonymization::retranslate()
{
    m_ui->retranslateUi(this);
}

QList<LogAnonymization::Rule> LogAnonymization::value() const
{
    QList<Rule> result;
    QTreeWidgetItem* item = m_ui->list->topLevelItem(0);
    for (int i = 1; item != nullptr; item = m_ui->list->topLevelItem(i++)) {
        QString regex = item->text(0).trimmed();
        if (!regex.isEmpty()) {
            result.append({ regex, item->text(1).trimmed() });
        }
    }

    return result;
}

void LogAnonymization::validateItem(QTreeWidgetItem* item)
{
    const QRegularExpression regex(item->text(0).trimmed());
    if (item->text(0).trimmed().isEmpty() || regex.isValid()) {
        item->setBackground(0, QBrush());
    } else {
        item->setBackground(0, QBrush(QColor(255, 0, 0, 80)));
    }
}
