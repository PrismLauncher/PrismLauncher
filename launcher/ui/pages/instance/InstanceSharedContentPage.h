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

#include <QMap>
#include <QWidget>

#include "ui/pages/BasePage.h"

class MinecraftInstance;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;

class InstanceSharedContentPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit InstanceSharedContentPage(MinecraftInstance* instance, QWidget* parent = nullptr);

    QString displayName() const override { return tr("Shared Content"); }
    QIcon icon() const override;
    QString id() const override { return "shared-content"; }
    QString helpPage() const override { return "Shared-Content"; }
    bool apply() override;
    void openedImpl() override;
    void retranslate() override;

   private slots:
    void createGroup();
    void openGroup();
    void updateEnabledState();

   private:
    QStringList selectedCategories() const;
    QStringList normalizedLines(QPlainTextEdit* editor) const;
    void loadSettings();
    void synchronizeGroup();
    void refreshGroups(const QString& select = QString());
    void showError(const QString& operation, const QString& error);

   private:
    MinecraftInstance* m_instance = nullptr;
    QLabel* m_description = nullptr;
    QCheckBox* m_enabled = nullptr;
    QLabel* m_groupLabel = nullptr;
    QComboBox* m_group = nullptr;
    QPushButton* m_createGroupButton = nullptr;
    QPushButton* m_openGroupButton = nullptr;
    QGroupBox* m_categoriesBox = nullptr;
    QMap<QString, QCheckBox*> m_categoryChecks;
    QLabel* m_excludedOptionsLabel = nullptr;
    QGroupBox* m_optionsBox = nullptr;
    QPlainTextEdit* m_excludedOptions = nullptr;
    QLabel* m_customPathsLabel = nullptr;
    QGroupBox* m_advancedBox = nullptr;
    QPlainTextEdit* m_customPaths = nullptr;
    QLabel* m_advancedWarning = nullptr;
    QString m_loadedGroup;
};
