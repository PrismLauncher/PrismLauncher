// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"

class MinecraftInstance;
class QLineEdit;

namespace Ui {
class PathsPage;
}

class PathsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit PathsPage(MinecraftInstance* instance, QWidget* parent = nullptr);
    ~PathsPage() override;

    QString displayName() const override { return tr("Paths"); }
    QIcon icon() const override { return QIcon::fromTheme("viewfolder"); }
    QString id() const override { return "paths"; }
    void openedImpl() override;
    void retranslate() override;

   private slots:
    void on_modsDirBrowseBtn_clicked();
    void on_resourcePacksDirBrowseBtn_clicked();
    void on_shaderPacksDirBrowseBtn_clicked();
    void on_worldsDirBrowseBtn_clicked();
    void on_screenshotsDirBrowseBtn_clicked();
    void on_modsDirResetBtn_clicked();
    void on_resourcePacksDirResetBtn_clicked();
    void on_shaderPacksDirResetBtn_clicked();
    void on_worldsDirResetBtn_clicked();
    void on_screenshotsDirResetBtn_clicked();

   private:
    void loadPaths();
    void browseForDirectory(const QString& path, QLineEdit* pathEdit, const QString& title);
    void changeDirectory(const QString& path, const QString& selected, QLineEdit* pathEdit);

    Ui::PathsPage* ui;
    MinecraftInstance* m_instance;
};
