// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "SharedContentPage.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "DesktopServices.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/SettingsObject.h"
#include "shared/SharedContentManager.h"

SharedContentPage::SharedContentPage(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_description = new QLabel(this);
    m_description->setWordWrap(true);
    layout->addWidget(m_description);

    auto* rootLayout = new QHBoxLayout;
    m_rootLabel = new QLabel(this);
    m_rootPath = new QLineEdit(this);
    m_rootPath->setReadOnly(true);
    m_rootPath->setClearButtonEnabled(false);
    m_changeRootButton = new QPushButton(this);
    m_openRootButton = new QPushButton(this);
    rootLayout->addWidget(m_rootLabel);
    rootLayout->addWidget(m_rootPath, 1);
    rootLayout->addWidget(m_changeRootButton);
    rootLayout->addWidget(m_openRootButton);
    layout->addLayout(rootLayout);

    m_groups = new QListWidget(this);
    m_groups->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_groups, 1);

    auto* buttonLayout = new QHBoxLayout;
    m_createButton = new QPushButton(this);
    m_renameButton = new QPushButton(this);
    m_deleteButton = new QPushButton(this);
    m_openButton = new QPushButton(this);
    m_repairButton = new QPushButton(this);
    buttonLayout->addWidget(m_createButton);
    buttonLayout->addWidget(m_renameButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_openButton);
    buttonLayout->addWidget(m_repairButton);
    layout->addLayout(buttonLayout);

    connect(m_createButton, &QPushButton::clicked, this, &SharedContentPage::createGroup);
    connect(m_renameButton, &QPushButton::clicked, this, &SharedContentPage::renameGroup);
    connect(m_deleteButton, &QPushButton::clicked, this, &SharedContentPage::deleteGroup);
    connect(m_changeRootButton, &QPushButton::clicked, this, &SharedContentPage::changeRoot);
    connect(m_openRootButton, &QPushButton::clicked, this, &SharedContentPage::openRoot);
    connect(m_openButton, &QPushButton::clicked, this, &SharedContentPage::openGroup);
    connect(m_repairButton, &QPushButton::clicked, this, &SharedContentPage::repairGroup);
    connect(m_groups, &QListWidget::itemSelectionChanged, this, &SharedContentPage::updateButtons);
    connect(m_groups, &QListWidget::itemDoubleClicked, this, [this] { openGroup(); });

    retranslate();
    refreshGroups();
}

QIcon SharedContentPage::icon() const
{
    auto icon = QIcon::fromTheme("shared-content");
    if (icon.isNull()) {
        icon = QIcon::fromTheme("folder-sync");
    }
    if (icon.isNull()) {
        icon = QIcon::fromTheme("folder");
    }
    return icon;
}

void SharedContentPage::openedImpl()
{
    refreshGroups(selectedGroup());
}

void SharedContentPage::retranslate()
{
    m_description->setText(tr(
        "Shared-content groups let multiple instances use the same selected settings and files. Mods and worlds always remain separate."));
    m_rootLabel->setText(tr("Shared folder:"));
    m_changeRootButton->setText(tr("Change…"));
    m_openRootButton->setText(tr("Open Folder"));
    m_createButton->setText(tr("Create"));
    m_renameButton->setText(tr("Rename"));
    m_deleteButton->setText(tr("Delete"));
    m_openButton->setText(tr("Open Group"));
    m_repairButton->setText(tr("Repair"));
}

QString SharedContentPage::selectedGroup() const
{
    auto* item = m_groups->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void SharedContentPage::refreshGroups(const QString& select)
{
    auto* manager = APPLICATION->sharedContent();
    m_groups->clear();
    if (!manager) {
        m_rootPath->clear();
        setEnabled(false);
        return;
    }

    setEnabled(true);
    m_rootPath->setText(manager->rootPath());
    const auto groups = manager->groups();
    for (const auto& group : groups) {
        auto* item = new QListWidgetItem(group, m_groups);
        item->setData(Qt::UserRole, group);
        if (group == select) {
            m_groups->setCurrentItem(item);
        }
    }
    updateButtons();
}

void SharedContentPage::createGroup()
{
    bool accepted = false;
    const QString name =
        QInputDialog::getText(this, tr("Create Shared Group"), tr("Group name:"), QLineEdit::Normal, QString(), &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }

    QString error;
    if (!APPLICATION->sharedContent()->createGroup(name, &error)) {
        showError(tr("create the shared group"), error);
        return;
    }
    refreshGroups(name);
}

void SharedContentPage::renameGroup()
{
    const QString oldName = selectedGroup();
    if (oldName.isEmpty()) {
        return;
    }

    bool accepted = false;
    const QString newName =
        QInputDialog::getText(this, tr("Rename Shared Group"), tr("Group name:"), QLineEdit::Normal, oldName, &accepted).trimmed();
    if (!accepted || newName.isEmpty() || newName == oldName) {
        return;
    }

    QList<MinecraftInstance*> affected;
    if (auto* instances = APPLICATION->instances()) {
        for (int i = 0; i < instances->count(); ++i) {
            auto* instance = instances->at(i);
            if (APPLICATION->sharedContent()->instanceGroup(instance) == oldName) {
                if (instance->isRunning()) {
                    showError(tr("rename the shared group"), tr("Stop %1 before renaming the group it uses.").arg(instance->name()));
                    return;
                }
                affected.append(instance);
            }
        }
    }

    QString error;
    if (!APPLICATION->sharedContent()->renameGroup(oldName, newName, &error)) {
        showError(tr("rename the shared group"), error);
        return;
    }
    QStringList failures;
    for (auto* instance : affected) {
        if (!APPLICATION->sharedContent()->configureInstance(instance, newName, APPLICATION->sharedContent()->instanceCategories(instance),
                                                             APPLICATION->sharedContent()->instanceCustomPaths(instance),
                                                             APPLICATION->sharedContent()->instanceExcludedOptions(instance),
                                                             SharedContent::MigrationPolicy::PreferShared, &error)) {
            failures.append(tr("%1: %2").arg(instance->name(), error));
        }
    }
    if (!failures.isEmpty()) {
        showError(tr("update every instance after renaming the group"), failures.join('\n'));
    }
    refreshGroups(newName);
}

void SharedContentPage::deleteGroup()
{
    const QString name = selectedGroup();
    if (name.isEmpty()) {
        return;
    }

    bool used = false;
    if (auto* instances = APPLICATION->instances()) {
        for (int i = 0; i < instances->count(); ++i) {
            if (APPLICATION->sharedContent()->instanceGroup(instances->at(i)) == name) {
                used = true;
                break;
            }
        }
    }
    if (used) {
        showError(tr("delete the shared group"), tr("Disconnect every instance that uses this group first."));
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Delete Shared Group"), tr("Delete the shared group “%1” and all content stored in it? This cannot be undone.").arg(name),
        QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!APPLICATION->sharedContent()->deleteGroup(name, true, &error)) {
        showError(tr("delete the shared group"), error);
        return;
    }
    refreshGroups();
}

void SharedContentPage::changeRoot()
{
    if (auto* instances = APPLICATION->instances()) {
        for (int i = 0; i < instances->count(); ++i) {
            if (!APPLICATION->sharedContent()->instanceGroup(instances->at(i)).isEmpty()) {
                showError(tr("change the shared folder"),
                          tr("Disconnect all instances from shared-content groups before changing the shared folder."));
                return;
            }
        }
    }

    const QString selected = QFileDialog::getExistingDirectory(this, tr("Select Shared Content Folder"), m_rootPath->text());
    if (selected.isEmpty() || QDir::cleanPath(selected) == QDir::cleanPath(m_rootPath->text())) {
        return;
    }
    if (!APPLICATION->sharedContent()->groups().isEmpty()) {
        const auto answer = QMessageBox::question(
            this, tr("Change Shared Content Folder"),
            tr("Existing groups will not be moved. You can return to the current folder later to use them again. Continue?"),
            QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Cancel);
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    QString error;
    if (!APPLICATION->sharedContent()->setRootPath(selected, &error)) {
        showError(tr("change the shared folder"), error);
        return;
    }
    APPLICATION->settings()->set("SharedContentDir", selected);
    refreshGroups();
}

void SharedContentPage::openRoot()
{
    if (!DesktopServices::openPath(APPLICATION->sharedContent()->rootPath(), true)) {
        showError(tr("open the shared folder"), tr("The folder could not be opened."));
    }
}

void SharedContentPage::openGroup()
{
    const QString name = selectedGroup();
    if (name.isEmpty()) {
        return;
    }
    if (!DesktopServices::openPath(APPLICATION->sharedContent()->groupPath(name), true)) {
        showError(tr("open the shared group"), tr("The folder could not be opened."));
    }
}

void SharedContentPage::repairGroup()
{
    const QString name = selectedGroup();
    if (name.isEmpty()) {
        return;
    }

    int repaired = 0;
    QStringList failures;
    if (auto* instances = APPLICATION->instances()) {
        QStringList runningInstances;
        for (int i = 0; i < instances->count(); ++i) {
            auto* instance = instances->at(i);
            if (instance->isRunning() && APPLICATION->sharedContent()->instanceGroup(instance) == name) {
                runningInstances.append(instance->name());
            }
        }
        if (!runningInstances.isEmpty()) {
            showError(tr("repair the shared group"),
                      tr("Stop every running instance in this group before repairing it: %1").arg(runningInstances.join(", ")));
            return;
        }

        for (int i = 0; i < instances->count(); ++i) {
            auto* instance = instances->at(i);
            if (APPLICATION->sharedContent()->instanceGroup(instance) != name) {
                continue;
            }
            QString instanceError;
            if (instance->isRunning()) {
                failures.append(tr("%1: stop the instance before repairing its shared folders.").arg(instance->name()));
                continue;
            }
            if (APPLICATION->sharedContent()->repairInstance(instance, &instanceError)) {
                ++repaired;
            } else {
                failures.append(tr("%1: %2").arg(instance->name(), instanceError));
            }
        }
    }
    if (!failures.isEmpty()) {
        showError(tr("repair every instance in the shared group"), failures.join('\n'));
        return;
    }
    QMessageBox::information(this, tr("Shared Group Repaired"),
                             tr("Checked and repaired %n instance(s) using “%1”.", nullptr, repaired).arg(name));
    refreshGroups(name);
}

void SharedContentPage::updateButtons()
{
    const bool selected = !selectedGroup().isEmpty();
    m_renameButton->setEnabled(selected);
    m_deleteButton->setEnabled(selected);
    m_openButton->setEnabled(selected);
    m_repairButton->setEnabled(selected);
}

void SharedContentPage::showError(const QString& operation, const QString& error)
{
    QMessageBox::critical(this, tr("Shared Content Error"), tr("Could not %1:\n%2").arg(operation, error));
}
