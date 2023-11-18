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

#pragma once

#include <QDialog>
#include <QTreeWidgetItem>

#include "GitHubRelease.h"
#include "Version.h"

namespace Ui {
class SelectReleaseDialog;
}

class SelectReleaseDialog : public QDialog {
    Q_OBJECT

   public:
    explicit SelectReleaseDialog(const Version& cur_version, const QList<GitHubRelease>& releases, QWidget* parent = 0);
    ~SelectReleaseDialog();

    void loadReleases();
    void appendRelease(GitHubRelease const& release);
    GitHubRelease selectedRelease() { return m_selectedRelease; }
   private slots:
    GitHubRelease getRelease(QTreeWidgetItem* item);
    void selectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

   protected:
    QList<GitHubRelease> m_releases;
    GitHubRelease m_selectedRelease;
    Version m_currentVersion;

    Ui::SelectReleaseDialog* ui;
};

class SelectReleaseAssetDialog : public QDialog {
    Q_OBJECT
   public:
    explicit SelectReleaseAssetDialog(const QList<GitHubReleaseAsset>& assets, QWidget* parent = 0);
    ~SelectReleaseAssetDialog();

    void loadAssets();
    void appendAsset(GitHubReleaseAsset const& asset);
    GitHubReleaseAsset selectedAsset() { return m_selectedAsset; }
   private slots:
    GitHubReleaseAsset getAsset(QTreeWidgetItem* item);
    void selectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

   protected:
    QList<GitHubReleaseAsset> m_assets;
    GitHubReleaseAsset m_selectedAsset;

    Ui::SelectReleaseDialog* ui;
};
