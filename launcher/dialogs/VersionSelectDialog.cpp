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

#include <dialogs/ProgressDialog.h>
#include "CustomMessageBox.h"

#include <BaseVersion.h>
#include <BaseVersionList.h>
#include <tasks/Task.h>
#include <QDebug>
#include "Application.h"
#include <VersionProxyModel.h>
#include <widgets/VersionSelectWidget.h>

VersionSelectDialog::VersionSelectDialog(BaseVersionList *vlist, QString title, QWidget *parent, bool cancelable)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("VersionSelectDialog"));
    resize(400, 347);
    m_verticalLayout = new QVBoxLayout(this);
    m_verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    m_versionWidget = new VersionSelectWidget(parent);
    m_verticalLayout->addWidget(m_versionWidget);

    m_horizontalLayout = new QHBoxLayout();
    m_horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setObjectName(QStringLiteral("refreshButton"));
    m_horizontalLayout->addWidget(m_refreshButton);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setObjectName(QStringLiteral("buttonBox"));
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_horizontalLayout->addWidget(m_buttonBox);

    m_verticalLayout->addLayout(m_horizontalLayout);

    retranslate();

    QObject::connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QMetaObject::connectSlotsByName(this);
    setWindowModality(Qt::WindowModal);
    setWindowTitle(title);

    m_vlist = vlist;

    if (!cancelable)
    {
        m_buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
    }
}

void VersionSelectDialog::retranslate()
{
    // FIXME: overrides custom title given in constructor!
    setWindowTitle(tr("Choose Version"));
    m_refreshButton->setToolTip(tr("Reloads the version list."));
    m_refreshButton->setText(tr("&Refresh"));
}

void VersionSelectDialog::setCurrentVersion(const QString& version)
{
    m_currentVersion = version;
    m_versionWidget->setCurrentVersion(version);
}

void VersionSelectDialog::setEmptyString(QString emptyString)
{
    m_versionWidget->setEmptyString(emptyString);
}

void VersionSelectDialog::setEmptyErrorString(QString emptyErrorString)
{
    m_versionWidget->setEmptyErrorString(emptyErrorString);
}

void VersionSelectDialog::setResizeOn(int column)
{
    resizeOnColumn = column;
}

int VersionSelectDialog::exec()
{
    QDialog::open();
    m_versionWidget->initialize(m_vlist);
    if(resizeOnColumn != -1)
    {
        m_versionWidget->setResizeOn(resizeOnColumn);
    }
    return QDialog::exec();
}

void VersionSelectDialog::selectRecommended()
{
    m_versionWidget->selectRecommended();
}

BaseVersionPtr VersionSelectDialog::selectedVersion() const
{
    return m_versionWidget->selectedVersion();
}

void VersionSelectDialog::on_refreshButton_clicked()
{
    m_versionWidget->loadList();
}

void VersionSelectDialog::setExactFilter(BaseVersionList::ModelRoles role, QString filter)
{
    m_versionWidget->setExactFilter(role, filter);
}

void VersionSelectDialog::setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter)
{
    m_versionWidget->setFuzzyFilter(role, filter);
}
