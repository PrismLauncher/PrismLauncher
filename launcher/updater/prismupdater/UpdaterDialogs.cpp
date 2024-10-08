// SPDX-FileCopyrightText: 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#include "UpdaterDialogs.h"

#include "ui_SelectReleaseDialog.h"

#include <QTextBrowser>
#include "Markdown.h"
#include "StringUtils.h"

SelectReleaseDialog::SelectReleaseDialog(const Version& current_version, const QList<GitHubRelease>& releases, QWidget* parent)
    : QDialog(parent), m_releases(releases), m_currentVersion(current_version), ui(new Ui::SelectReleaseDialog)
{
    ui->setupUi(this);

    ui->changelogTextBrowser->setOpenExternalLinks(true);
    ui->changelogTextBrowser->setLineWrapMode(QTextBrowser::LineWrapMode::WidgetWidth);
    ui->changelogTextBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);

    ui->versionsTree->setColumnCount(2);

    ui->versionsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->versionsTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->versionsTree->setHeaderLabels({ tr("Version"), tr("Published Date") });
    ui->versionsTree->header()->setStretchLastSection(false);

    ui->eplainLabel->setText(tr("Select a version to install.\n"
                                "\n"
                                "Currently installed version: %1")
                                 .arg(m_currentVersion.toString()));

    loadReleases();

    connect(ui->versionsTree, &QTreeWidget::currentItemChanged, this, &SelectReleaseDialog::selectionChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SelectReleaseDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SelectReleaseDialog::reject);
}

SelectReleaseDialog::~SelectReleaseDialog()
{
    delete ui;
}

void SelectReleaseDialog::loadReleases()
{
    for (auto rls : m_releases) {
        appendRelease(rls);
    }
}

void SelectReleaseDialog::appendRelease(GitHubRelease const& release)
{
    auto rls_item = new QTreeWidgetItem(ui->versionsTree);
    rls_item->setText(0, release.tag_name);
    rls_item->setExpanded(true);
    rls_item->setText(1, release.published_at.toString());
    rls_item->setData(0, Qt::UserRole, QVariant(release.id));

    ui->versionsTree->addTopLevelItem(rls_item);
}

GitHubRelease SelectReleaseDialog::getRelease(QTreeWidgetItem* item)
{
    int id = item->data(0, Qt::UserRole).toInt();
    GitHubRelease release;
    for (auto rls : m_releases) {
        if (rls.id == id)
            release = rls;
    }
    return release;
}

void SelectReleaseDialog::selectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    GitHubRelease release = getRelease(current);
    QString body = markdownToHTML(release.body.toUtf8());
    m_selectedRelease = release;

    ui->changelogTextBrowser->setHtml(StringUtils::htmlListPatch(body));
}

SelectReleaseAssetDialog::SelectReleaseAssetDialog(const QList<GitHubReleaseAsset>& assets, QWidget* parent)
    : QDialog(parent), m_assets(assets), ui(new Ui::SelectReleaseDialog)
{
    ui->setupUi(this);

    ui->changelogTextBrowser->setOpenExternalLinks(true);
    ui->changelogTextBrowser->setLineWrapMode(QTextBrowser::LineWrapMode::WidgetWidth);
    ui->changelogTextBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);

    ui->versionsTree->setColumnCount(2);

    ui->versionsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->versionsTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->versionsTree->setHeaderLabels({ tr("Version"), tr("Published Date") });
    ui->versionsTree->header()->setStretchLastSection(false);

    ui->eplainLabel->setText(tr("Select a version to install."));

    ui->changelogTextBrowser->setHidden(true);

    loadAssets();

    connect(ui->versionsTree, &QTreeWidget::currentItemChanged, this, &SelectReleaseAssetDialog::selectionChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SelectReleaseAssetDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SelectReleaseAssetDialog::reject);
}

SelectReleaseAssetDialog::~SelectReleaseAssetDialog()
{
    delete ui;
}

void SelectReleaseAssetDialog::loadAssets()
{
    for (auto rls : m_assets) {
        appendAsset(rls);
    }
}

void SelectReleaseAssetDialog::appendAsset(GitHubReleaseAsset const& asset)
{
    auto rls_item = new QTreeWidgetItem(ui->versionsTree);
    rls_item->setText(0, asset.name);
    rls_item->setExpanded(true);
    rls_item->setText(1, asset.updated_at.toString());
    rls_item->setData(0, Qt::UserRole, QVariant(asset.id));

    ui->versionsTree->addTopLevelItem(rls_item);
}

GitHubReleaseAsset SelectReleaseAssetDialog::getAsset(QTreeWidgetItem* item)
{
    int id = item->data(0, Qt::UserRole).toInt();
    GitHubReleaseAsset selected_asset;
    for (auto asset : m_assets) {
        if (asset.id == id)
            selected_asset = asset;
    }
    return selected_asset;
}

void SelectReleaseAssetDialog::selectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    GitHubReleaseAsset asset = getAsset(current);
    m_selectedAsset = asset;
}
