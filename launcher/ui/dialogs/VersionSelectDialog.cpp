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

#include "VersionSelectDialog.h"

#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QDebug>

#include "ui/dialogs/ProgressDialog.h"
#include "ui/widgets/VersionSelectWidget.h"
#include "ui/dialogs/CustomMessageBox.h"

#include "BaseVersion.h"
#include "BaseVersionList.h"
#include "tasks/Task.h"
#include "Application.h"
#include "VersionProxyModel.h"

VersionSelectDialog::VersionSelectDialog(BaseVersionList *vlist, QString title, QWidget *parent, bool cancelable)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("VersionSelectDialog"));
    resize(400, 347);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout = new QVBoxLayout(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget = new VersionSelectWidget(parent);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout = new QHBoxLayout();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_refreshButton = new QPushButton(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_refreshButton->setObjectName(QStringLiteral("refreshButton"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_refreshButton);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_buttonBox = new QDialogButtonBox(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_buttonBox->setObjectName(QStringLiteral("buttonBox"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_buttonBox->setOrientation(Qt::Horizontal);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_buttonBox);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout->addLayout(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout);

    retranslate();

    QObject::connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QMetaObject::connectSlotsByName(this);
    setWindowModality(Qt::WindowModal);
    setWindowTitle(title);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_vlist = vlist;

    if (!cancelable)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
    }
}

void VersionSelectDialog::retranslate()
{
    // FIXME: overrides custom title given in constructor!
    setWindowTitle(tr("Choose Version"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_refreshButton->setToolTip(tr("Reloads the version list."));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_refreshButton->setText(tr("&Refresh"));
}

void VersionSelectDialog::setCurrentVersion(const QString& version)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersion = version;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->setCurrentVersion(version);
}

void VersionSelectDialog::setEmptyString(QString emptyString)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->setEmptyString(emptyString);
}

void VersionSelectDialog::setEmptyErrorString(QString emptyErrorString)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->setEmptyErrorString(emptyErrorString);
}

void VersionSelectDialog::setResizeOn(int column)
{
    resizeOnColumn = column;
}

int VersionSelectDialog::exec()
{
    QDialog::open();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->initialize(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_vlist);
    if(resizeOnColumn != -1)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->setResizeOn(resizeOnColumn);
    }
    return QDialog::exec();
}

void VersionSelectDialog::selectRecommended()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->selectRecommended();
}

BaseVersion::Ptr VersionSelectDialog::selectedVersion() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->selectedVersion();
}

void VersionSelectDialog::on_refreshButton_clicked()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->loadList();
}

void VersionSelectDialog::setExactFilter(BaseVersionList::ModelRoles role, QString filter)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->setExactFilter(role, filter);
}

void VersionSelectDialog::setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->setFuzzyFilter(role, filter);
}
