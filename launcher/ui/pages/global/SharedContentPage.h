// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

class SharedContentPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit SharedContentPage(QWidget* parent = nullptr);

    QString displayName() const override { return tr("Shared Content"); }
    QIcon icon() const override;
    QString id() const override { return "shared-content"; }
    QString helpPage() const override { return "Shared-Content"; }
    void openedImpl() override;
    void retranslate() override;

   private slots:
    void createGroup();
    void renameGroup();
    void deleteGroup();
    void changeRoot();
    void openRoot();
    void openGroup();
    void repairGroup();
    void updateButtons();

   private:
    QString selectedGroup() const;
    void refreshGroups(const QString& select = QString());
    void showError(const QString& operation, const QString& error);

   private:
    QLabel* m_description = nullptr;
    QLabel* m_rootLabel = nullptr;
    QLineEdit* m_rootPath = nullptr;
    QPushButton* m_changeRootButton = nullptr;
    QPushButton* m_openRootButton = nullptr;
    QListWidget* m_groups = nullptr;
    QPushButton* m_createButton = nullptr;
    QPushButton* m_renameButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_openButton = nullptr;
    QPushButton* m_repairButton = nullptr;
};
