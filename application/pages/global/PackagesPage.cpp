/* Copyright 2015-2019 MultiMC Contributors
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

#include "PackagesPage.h"
#include "ui_PackagesPage.h"

#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QRegularExpression>

#include "dialogs/ProgressDialog.h"
#include "VersionProxyModel.h"

#include "meta/Index.h"
#include "meta/VersionList.h"
#include "meta/Version.h"
#include "Env.h"
#include "MultiMC.h"

using namespace Meta;

static QString formatRequires(const VersionPtr &version)
{
    QStringList lines;
    auto & reqs = version->requires();
    auto iter = reqs.begin();
    while (iter != reqs.end())
    {
        auto &uid = iter->uid;
        auto &version = iter->equalsVersion;
        const QString readable = ENV.metadataIndex()->hasUid(uid) ? ENV.metadataIndex()->get(uid)->humanReadable() : uid;
        if(!version.isEmpty())
        {
            lines.append(QString("%1 (%2)").arg(readable, version));
        }
        else
        {
            lines.append(QString("%1").arg(readable));
        }
        iter++;
    }
    return lines.join('\n');
}

PackagesPage::PackagesPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PackagesPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();

    m_fileProxy = new QSortFilterProxyModel(this);
    m_fileProxy->setSortRole(Qt::DisplayRole);
    m_fileProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_fileProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_fileProxy->setFilterRole(Qt::DisplayRole);
    m_fileProxy->setFilterKeyColumn(0);
    m_fileProxy->sort(0);
    m_fileProxy->setSourceModel(ENV.metadataIndex().get());
    ui->indexView->setModel(m_fileProxy);

    m_filterProxy = new QSortFilterProxyModel(this);
    m_filterProxy->setSortRole(VersionList::SortRole);
    m_filterProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterProxy->setFilterRole(Qt::DisplayRole);
    m_filterProxy->setFilterKeyColumn(0);
    m_filterProxy->sort(0, Qt::DescendingOrder);
    ui->versionsView->setModel(m_filterProxy);

    m_versionProxy = new VersionProxyModel(this);
    m_filterProxy->setSourceModel(m_versionProxy);

    connect(ui->indexView->selectionModel(), &QItemSelectionModel::currentChanged, this, &PackagesPage::updateCurrentVersionList);
    connect(ui->versionsView->selectionModel(), &QItemSelectionModel::currentChanged, this, &PackagesPage::updateVersion);
    connect(m_filterProxy, &QSortFilterProxyModel::dataChanged, this, &PackagesPage::versionListDataChanged);

    updateCurrentVersionList(QModelIndex());
    updateVersion();
}

PackagesPage::~PackagesPage()
{
    delete ui;
}

QIcon PackagesPage::icon() const
{
    return MMC->getThemedIcon("packages");
}

void PackagesPage::on_refreshIndexBtn_clicked()
{
    ENV.metadataIndex()->load(Net::Mode::Online);
}
void PackagesPage::on_refreshFileBtn_clicked()
{
    VersionListPtr list = ui->indexView->currentIndex().data(Index::ListPtrRole).value<VersionListPtr>();
    if (!list)
    {
        return;
    }
    list->load(Net::Mode::Online);
}
void PackagesPage::on_refreshVersionBtn_clicked()
{
    VersionPtr version = ui->versionsView->currentIndex().data(VersionList::VersionPtrRole).value<VersionPtr>();
    if (!version)
    {
        return;
    }
    version->load(Net::Mode::Online);
}

void PackagesPage::on_fileSearchEdit_textChanged(const QString &search)
{
    if (search.isEmpty())
    {
        m_fileProxy->setFilterFixedString(QString());
    }
    else
    {
        QStringList parts = search.split(' ');
        std::transform(parts.begin(), parts.end(), parts.begin(), &QRegularExpression::escape);
        m_fileProxy->setFilterRegExp(".*" + parts.join(".*") + ".*");
    }
}
void PackagesPage::on_versionSearchEdit_textChanged(const QString &search)
{
    if (search.isEmpty())
    {
        m_filterProxy->setFilterFixedString(QString());
    }
    else
    {
        QStringList parts = search.split(' ');
        std::transform(parts.begin(), parts.end(), parts.begin(), &QRegularExpression::escape);
        m_filterProxy->setFilterRegExp(".*" + parts.join(".*") + ".*");
    }
}

void PackagesPage::updateCurrentVersionList(const QModelIndex &index)
{
    if (index.isValid())
    {
        VersionListPtr list = index.data(Index::ListPtrRole).value<VersionListPtr>();
        ui->versionsBox->setEnabled(true);
        ui->refreshFileBtn->setEnabled(true);
        ui->fileUidLabel->setEnabled(true);
        ui->fileUid->setText(list->uid());
        ui->fileNameLabel->setEnabled(true);
        ui->fileName->setText(list->name());
        m_versionProxy->setSourceModel(list.get());
        ui->refreshFileBtn->setText(tr("Refresh %1").arg(list->humanReadable()));
        list->load(Net::Mode::Offline);
    }
    else
    {
        ui->versionsBox->setEnabled(false);
        ui->refreshFileBtn->setEnabled(false);
        ui->fileUidLabel->setEnabled(false);
        ui->fileUid->clear();
        ui->fileNameLabel->setEnabled(false);
        ui->fileName->clear();
        m_versionProxy->setSourceModel(nullptr);
        ui->refreshFileBtn->setText(tr("Refresh"));
    }
}

void PackagesPage::versionListDataChanged(const QModelIndex &tl, const QModelIndex &br)
{
    if (QItemSelection(tl, br).contains(ui->versionsView->currentIndex()))
    {
        updateVersion();
    }
}

void PackagesPage::updateVersion()
{
    VersionPtr version = std::dynamic_pointer_cast<Version>(
                ui->versionsView->currentIndex().data(VersionList::VersionPointerRole).value<BaseVersionPtr>());
    if (version)
    {
        ui->refreshVersionBtn->setEnabled(true);
        ui->versionVersionLabel->setEnabled(true);
        ui->versionVersion->setText(version->version());
        ui->versionTimeLabel->setEnabled(true);
        ui->versionTime->setText(version->time().toString("yyyy-MM-dd HH:mm"));
        ui->versionTypeLabel->setEnabled(true);
        ui->versionType->setText(version->type());
        ui->versionRequiresLabel->setEnabled(true);
        ui->versionRequires->setText(formatRequires(version));
        ui->refreshVersionBtn->setText(tr("Refresh %1").arg(version->version()));
    }
    else
    {
        ui->refreshVersionBtn->setEnabled(false);
        ui->versionVersionLabel->setEnabled(false);
        ui->versionVersion->clear();
        ui->versionTimeLabel->setEnabled(false);
        ui->versionTime->clear();
        ui->versionTypeLabel->setEnabled(false);
        ui->versionType->clear();
        ui->versionRequiresLabel->setEnabled(false);
        ui->versionRequires->clear();
        ui->refreshVersionBtn->setText(tr("Refresh"));
    }
}

void PackagesPage::openedImpl()
{
    ENV.metadataIndex()->load(Net::Mode::Offline);
}
