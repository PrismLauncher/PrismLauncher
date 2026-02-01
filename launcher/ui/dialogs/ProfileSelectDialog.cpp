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

#include "ProfileSelectDialog.h"
#include "ui_ProfileSelectDialog.h"

#include <QDebug>
#include <QIdentityProxyModel>
#include <QItemSelectionModel>
#include <QPushButton>

#include "Application.h"

// HACK: hide checkboxes from AccountList
class HideCheckboxProxyModel : public QIdentityProxyModel {
   public:
    using QIdentityProxyModel::QIdentityProxyModel;

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (role == Qt::CheckStateRole) {
            return {};
        }

        return QIdentityProxyModel::data(index, role);
    }
};

ProfileSelectDialog::ProfileSelectDialog(const QString& message, int flags, QWidget* parent)
    : QDialog(parent), ui(new Ui::ProfileSelectDialog)
{
    ui->setupUi(this);

    m_accounts = APPLICATION->accounts();

    auto proxy = new HideCheckboxProxyModel(ui->view);
    proxy->setSourceModel(m_accounts);
    ui->view->setModel(proxy);

    // Set the message label.
    ui->msgLabel->setVisible(!message.isEmpty());
    ui->msgLabel->setText(message);

    // Flags...
    ui->globalDefaultCheck->setVisible(flags & GlobalDefaultCheckbox);
    ui->instDefaultCheck->setVisible(flags & InstanceDefaultCheckbox);
    qDebug() << flags;

    // Select the first entry in the list.
    ui->view->setCurrentIndex(ui->view->model()->index(0, 0));

    connect(ui->view, &QAbstractItemView::doubleClicked, this, &ProfileSelectDialog::on_buttonBox_accepted);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

ProfileSelectDialog::~ProfileSelectDialog()
{
    delete ui;
}

MinecraftAccountPtr ProfileSelectDialog::selectedAccount() const
{
    return m_selected;
}

bool ProfileSelectDialog::useAsGlobalDefault() const
{
    return ui->globalDefaultCheck->isChecked();
}

bool ProfileSelectDialog::useAsInstDefaullt() const
{
    return ui->instDefaultCheck->isChecked();
}

void ProfileSelectDialog::on_buttonBox_accepted()
{
    QModelIndexList selection = ui->view->selectionModel()->selectedIndexes();
    if (selection.size() > 0) {
        QModelIndex selected = selection.first();
        m_selected = selected.data(AccountList::PointerRole).value<MinecraftAccountPtr>();
    }
    close();
}

void ProfileSelectDialog::on_buttonBox_rejected()
{
    close();
}
