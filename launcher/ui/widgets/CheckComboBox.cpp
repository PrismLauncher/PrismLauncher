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

#include "CheckComboBox.h"

#include <QAbstractItemView>
#include <QBoxLayout>
#include <QEvent>
#include <QIdentityProxyModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QMouseEvent>
#include <QStringList>
#include <QStylePainter>

class CheckComboModel : public QIdentityProxyModel {
    Q_OBJECT

   public:
    explicit CheckComboModel(QObject* parent = nullptr) : QIdentityProxyModel(parent) {}

    virtual Qt::ItemFlags flags(const QModelIndex& index) const { return QIdentityProxyModel::flags(index) | Qt::ItemIsUserCheckable; }
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const
    {
        if (role == Qt::CheckStateRole) {
            auto txt = QIdentityProxyModel::data(index, Qt::DisplayRole).toString();
            return checked.contains(txt) ? Qt::Checked : Qt::Unchecked;
        }
        if (role == Qt::DisplayRole)
            return QIdentityProxyModel::data(index, Qt::DisplayRole);
        return {};
    }
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole)
    {
        if (role == Qt::CheckStateRole) {
            auto txt = QIdentityProxyModel::data(index, Qt::DisplayRole).toString();
            if (checked.contains(txt)) {
                checked.removeOne(txt);
            } else {
                checked.push_back(txt);
            }
            emit dataChanged(index, index);
            emit checkStateChanged();
            return true;
        }
        return QIdentityProxyModel::setData(index, value, role);
    }
    QStringList getChecked() { return checked; }

   signals:
    void checkStateChanged();

   private:
    QStringList checked;
};

CheckComboBox::CheckComboBox(QWidget* parent) : QComboBox(parent), m_separator(", ")
{
    view()->installEventFilter(this);
    view()->window()->installEventFilter(this);
    view()->viewport()->installEventFilter(this);
    this->installEventFilter(this);
}

void CheckComboBox::setSourceModel(QAbstractItemModel* new_model)
{
    auto proxy = new CheckComboModel(this);
    proxy->setSourceModel(new_model);
    model()->disconnect(this);
    QComboBox::setModel(proxy);
    connect(this, QOverload<int>::of(&QComboBox::activated), this, &CheckComboBox::toggleCheckState);
    connect(proxy, &CheckComboModel::checkStateChanged, this, &CheckComboBox::emitCheckedItemsChanged);
    connect(model(), &CheckComboModel::rowsInserted, this, &CheckComboBox::emitCheckedItemsChanged);
    connect(model(), &CheckComboModel::rowsRemoved, this, &CheckComboBox::emitCheckedItemsChanged);
}

void CheckComboBox::hidePopup()
{
    if (!containerMousePress)
        QComboBox::hidePopup();
}

void CheckComboBox::emitCheckedItemsChanged()
{
    emit checkedItemsChanged(checkedItems());
}

QString CheckComboBox::defaultText() const
{
    return m_default_text;
}

void CheckComboBox::setDefaultText(const QString& text)
{
    m_default_text = text;
}

QString CheckComboBox::separator() const
{
    return m_separator;
}

void CheckComboBox::setSeparator(const QString& separator)
{
    m_separator = separator;
}

bool CheckComboBox::eventFilter(QObject* receiver, QEvent* event)
{
    switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (receiver == this && (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)) {
                showPopup();
                return true;
            } else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Escape) {
                QComboBox::hidePopup();
                return (keyEvent->key() != Qt::Key_Escape);
            }
            break;
        }
        case QEvent::MouseButtonPress: {
            auto ev = static_cast<QMouseEvent*>(event);
            containerMousePress = ev && view()->indexAt(ev->pos()).isValid();
            break;
        }
        case QEvent::Wheel:
            return receiver == this;
        default:
            break;
    }
    return false;
}

void CheckComboBox::toggleCheckState(int index)
{
    QVariant value = itemData(index, Qt::CheckStateRole);
    if (value.isValid()) {
        Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
        setItemData(index, (state == Qt::Unchecked ? Qt::Checked : Qt::Unchecked), Qt::CheckStateRole);
    }
    emitCheckedItemsChanged();
}

Qt::CheckState CheckComboBox::itemCheckState(int index) const
{
    return static_cast<Qt::CheckState>(itemData(index, Qt::CheckStateRole).toInt());
}

void CheckComboBox::setItemCheckState(int index, Qt::CheckState state)
{
    setItemData(index, state, Qt::CheckStateRole);
}

QStringList CheckComboBox::checkedItems() const
{
    if (model())
        return dynamic_cast<CheckComboModel*>(model())->getChecked();
    return {};
}

void CheckComboBox::setCheckedItems(const QStringList& items)
{
    foreach (auto text, items) {
        auto index = findText(text);
        setItemCheckState(index, index != -1 ? Qt::Checked : Qt::Unchecked);
    }
}

void CheckComboBox::paintEvent(QPaintEvent*)
{
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QStringList items = checkedItems();
    if (items.isEmpty())
        opt.currentText = defaultText();
    else
        opt.currentText = items.join(separator());
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

#include "CheckComboBox.moc"