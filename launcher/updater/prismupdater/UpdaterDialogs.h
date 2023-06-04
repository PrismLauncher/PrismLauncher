#pragma once

#include <QDialog>
#include <QTreeWidgetItem>

#include "Version.h"
#include "updater/prismupdater/GitHubRelease.h"

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
