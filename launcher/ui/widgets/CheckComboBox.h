// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#pragma once

#include <QComboBox>
#include <QLineEdit>

class CheckComboBox : public QComboBox {
    Q_OBJECT

   public:
    explicit CheckComboBox(QWidget* parent = nullptr);
    virtual ~CheckComboBox() = default;

    void hidePopup() override;

    QString defaultText() const;
    void setDefaultText(const QString& text);

    Qt::CheckState itemCheckState(int index) const;
    void setItemCheckState(int index, Qt::CheckState state);

    QString separator() const;
    void setSeparator(const QString& separator);

    QStringList checkedItems() const;

    void setSourceModel(QAbstractItemModel* model);

   public slots:
    void setCheckedItems(const QStringList& items);

   signals:
    void checkedItemsChanged(const QStringList& items);

   protected:
    void paintEvent(QPaintEvent*) override;

   private:
    void emitCheckedItemsChanged();
    bool eventFilter(QObject* receiver, QEvent* event) override;
    void toggleCheckState(int index);

   private:
    QString m_default_text;
    QString m_separator;
    bool containerMousePress;
};