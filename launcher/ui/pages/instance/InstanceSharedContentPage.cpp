// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "InstanceSharedContentPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "Application.h"
#include "DesktopServices.h"
#include "minecraft/MinecraftInstance.h"
#include "shared/SharedContentManager.h"

namespace {
const QStringList& categoryIds()
{
    static const QStringList ids{ "options", "screenshots", "resourcepacks",   "texturepacks",    "shaderpacks",
                                  "config",  "servers",     "command_history", "creative_hotbar", "global_datapacks" };
    return ids;
}

QString categoryName(const QString& id)
{
    if (id == "options")
        return InstanceSharedContentPage::tr("Game options");
    if (id == "screenshots")
        return InstanceSharedContentPage::tr("Screenshots");
    if (id == "resourcepacks")
        return InstanceSharedContentPage::tr("Resource packs");
    if (id == "texturepacks")
        return InstanceSharedContentPage::tr("Legacy texture packs");
    if (id == "shaderpacks")
        return InstanceSharedContentPage::tr("Shader packs");
    if (id == "config")
        return InstanceSharedContentPage::tr("Config files");
    if (id == "servers")
        return InstanceSharedContentPage::tr("Multiplayer servers");
    if (id == "command_history")
        return InstanceSharedContentPage::tr("Command history");
    if (id == "creative_hotbar")
        return InstanceSharedContentPage::tr("Creative hotbars");
    if (id == "global_datapacks")
        return InstanceSharedContentPage::tr("Global data packs");
    return id;
}
}

InstanceSharedContentPage::InstanceSharedContentPage(MinecraftInstance* instance, QWidget* parent) : QWidget(parent), m_instance(instance)
{
    auto* layout = new QVBoxLayout(this);

    m_description = new QLabel(this);
    m_description->setWordWrap(true);
    layout->addWidget(m_description);

    m_enabled = new QCheckBox(this);
    layout->addWidget(m_enabled);

    auto* groupLayout = new QHBoxLayout;
    m_groupLabel = new QLabel(this);
    m_group = new QComboBox(this);
    m_group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_createGroupButton = new QPushButton(this);
    m_openGroupButton = new QPushButton(this);
    groupLayout->addWidget(m_groupLabel);
    groupLayout->addWidget(m_group, 1);
    groupLayout->addWidget(m_createGroupButton);
    groupLayout->addWidget(m_openGroupButton);
    layout->addLayout(groupLayout);

    m_categoriesBox = new QGroupBox(this);
    auto* categoriesLayout = new QGridLayout(m_categoriesBox);
    int index = 0;
    for (const auto& id : categoryIds()) {
        auto* check = new QCheckBox(m_categoriesBox);
        check->setProperty("categoryId", id);
        categoriesLayout->addWidget(check, index / 2, index % 2);
        m_categoryChecks.insert(id, check);
        ++index;
    }
    layout->addWidget(m_categoriesBox);

    m_optionsBox = new QGroupBox(this);
    auto* optionsLayout = new QVBoxLayout(m_optionsBox);
    m_excludedOptionsLabel = new QLabel(m_optionsBox);
    m_excludedOptionsLabel->setWordWrap(true);
    m_excludedOptions = new QPlainTextEdit(m_optionsBox);
    m_excludedOptions->setMaximumHeight(90);
    optionsLayout->addWidget(m_excludedOptionsLabel);
    optionsLayout->addWidget(m_excludedOptions);
    layout->addWidget(m_optionsBox);

    m_advancedBox = new QGroupBox(this);
    auto* advancedLayout = new QVBoxLayout(m_advancedBox);
    m_customPathsLabel = new QLabel(m_advancedBox);
    m_customPathsLabel->setWordWrap(true);
    m_customPaths = new QPlainTextEdit(m_advancedBox);
    m_customPaths->setMaximumHeight(100);
    m_advancedWarning = new QLabel(m_advancedBox);
    m_advancedWarning->setWordWrap(true);
    advancedLayout->addWidget(m_customPathsLabel);
    advancedLayout->addWidget(m_customPaths);
    advancedLayout->addWidget(m_advancedWarning);
    layout->addWidget(m_advancedBox);
    layout->addStretch(1);

    connect(m_enabled, &QCheckBox::toggled, this, &InstanceSharedContentPage::enabledToggled);
    connect(m_group, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InstanceSharedContentPage::updateEnabledState);
    connect(m_createGroupButton, &QPushButton::clicked, this, &InstanceSharedContentPage::createGroup);
    connect(m_openGroupButton, &QPushButton::clicked, this, &InstanceSharedContentPage::openGroup);

    retranslate();
    refreshGroups();
    loadSettings();
}

QIcon InstanceSharedContentPage::icon() const
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

void InstanceSharedContentPage::openedImpl()
{
    synchronizeGroup();
    const QString selected = m_enabled->isChecked() ? m_group->currentData().toString() : m_loadedGroup;
    refreshGroups(selected);
}

void InstanceSharedContentPage::retranslate()
{
    m_description->setText(
        tr("Share selected game settings and files with other instances in the same group. Mods and worlds are never shared."));
    m_enabled->setText(tr("Use shared content for this instance"));
    m_groupLabel->setText(tr("Sharing group:"));
    m_createGroupButton->setText(tr("Create"));
    m_openGroupButton->setText(tr("Open Folder"));

    m_categoriesBox->setTitle(tr("Shared categories"));
    for (auto it = m_categoryChecks.cbegin(); it != m_categoryChecks.cend(); ++it) {
        it.value()->setText(categoryName(it.key()));
    }
    m_categoryChecks.value("config")->setToolTip(
        tr("Config files can be incompatible between different Minecraft versions, mod loaders, and mod sets."));

    m_optionsBox->setTitle(tr("Per-instance game option overrides"));
    m_excludedOptionsLabel->setText(
        tr("Option keys that should remain local to this instance, one per line. These apply only when Game options is selected."));
    m_excludedOptions->setPlaceholderText(tr("Example: guiScale"));

    m_advancedBox->setTitle(tr("Advanced custom paths"));
    m_customPathsLabel->setText(
        tr("Relative paths inside the Minecraft folder, one per line. End a directory path with ‘/’; other entries are treated as files."));
    m_customPaths->setPlaceholderText(tr("schematics/\njourneymap/\nexample.dat"));
    m_advancedWarning->setText(
        tr("Custom paths may be incompatible between instances. Paths outside the game folder, mods, and saves are not allowed."));
}

void InstanceSharedContentPage::refreshGroups(const QString& select)
{
    const QSignalBlocker blocker(m_group);
    const QString current = select.isEmpty() ? m_group->currentData().toString() : select;
    m_group->clear();
    if (auto* manager = APPLICATION->sharedContent()) {
        for (const auto& name : manager->groups()) {
            m_group->addItem(name, name);
        }
    }
    const int index = m_group->findData(current);
    if (index >= 0) {
        m_group->setCurrentIndex(index);
    } else if (!current.isEmpty()) {
        m_group->setCurrentIndex(-1);
    }
    updateEnabledState();
}

void InstanceSharedContentPage::loadSettings()
{
    auto* manager = APPLICATION->sharedContent();
    if (!manager) {
        setEnabled(false);
        return;
    }
    const QString group = manager->instanceGroup(m_instance);
    {
        const QSignalBlocker blocker(m_enabled);
        m_enabled->setChecked(!group.isEmpty());
    }
    refreshGroups(group);

    const QStringList selected = SharedContent::Manager::serializeCategories(manager->instanceCategories(m_instance));
    for (auto it = m_categoryChecks.cbegin(); it != m_categoryChecks.cend(); ++it) {
        it.value()->setChecked(selected.contains(it.key()));
    }
    m_categorySelectionInitialized = !group.isEmpty() || !selected.isEmpty();
    m_excludedOptions->setPlainText(manager->instanceExcludedOptions(m_instance).join('\n'));
    QStringList customPaths;
    for (const auto& path : manager->instanceCustomPaths(m_instance)) {
        customPaths.append(path.relativePath + (path.directory && !path.relativePath.endsWith('/') ? "/" : ""));
    }
    m_customPaths->setPlainText(customPaths.join('\n'));
    m_loadedGroup = group;
    updateEnabledState();
}

void InstanceSharedContentPage::synchronizeGroup()
{
    auto* manager = APPLICATION->sharedContent();
    if (!manager) {
        return;
    }
    const QString group = manager->instanceGroup(m_instance);
    if (group == m_loadedGroup) {
        return;
    }

    const QString selected = m_group->currentData().toString();
    if (m_enabled->isChecked() && selected == m_loadedGroup) {
        refreshGroups(group);
        if (group.isEmpty()) {
            m_enabled->setChecked(false);
        }
    } else {
        refreshGroups(selected);
    }
    m_loadedGroup = group;
}

QStringList InstanceSharedContentPage::selectedCategories() const
{
    QStringList result;
    for (const auto& id : categoryIds()) {
        if (m_categoryChecks.value(id)->isChecked()) {
            result.append(id);
        }
    }
    return result;
}

QStringList InstanceSharedContentPage::normalizedLines(QPlainTextEdit* editor) const
{
    QStringList result;
    const auto lines = editor->toPlainText().split('\n');
    for (const auto& line : lines) {
        const QString value = line.trimmed();
        if (!value.isEmpty() && !result.contains(value)) {
            result.append(value);
        }
    }
    return result;
}

bool InstanceSharedContentPage::apply()
{
    auto* manager = APPLICATION->sharedContent();
    if (!manager) {
        showError(tr("save shared-content settings"), tr("The shared-content service is unavailable."));
        return false;
    }

    synchronizeGroup();

    const QString group = m_enabled->isChecked() ? m_group->currentData().toString() : QString();
    if (m_enabled->isChecked() && group.isEmpty()) {
        showError(tr("enable shared content"), tr("Create or select a sharing group first."));
        return false;
    }

    QString error;
    const QString oldGroup = manager->instanceGroup(m_instance);
    if (group.isEmpty()) {
        if (oldGroup.isEmpty()) {
            return true;
        }

        QMessageBox box(QMessageBox::Question, tr("Disconnect Shared Content"),
                        tr("How should the shared content be disconnected from this instance?"), QMessageBox::NoButton, this);
        auto* copyBack = box.addButton(tr("Copy Shared Content Back"), QMessageBox::AcceptRole);
        box.addButton(tr("Keep Current Local Files (Shared Folders Become Empty)"), QMessageBox::DestructiveRole);
        auto* cancel = box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(copyBack);
        box.exec();
        if (!box.clickedButton() || box.clickedButton() == cancel) {
            return false;
        }
        if (m_instance->isRunning()) {
            showError(tr("disconnect shared content"), tr("Stop the instance before changing its shared content."));
            return false;
        }
        const bool shouldCopyBack = box.clickedButton() == copyBack;
        if (!manager->disconnectInstance(m_instance, shouldCopyBack, &error)) {
            showError(tr("disconnect shared content"), error);
            return false;
        }
        m_loadedGroup.clear();
        return true;
    }

    const QStringList categoryIds = selectedCategories();
    const SharedContent::Categories categories = SharedContent::Manager::deserializeCategories(categoryIds);
    const QStringList excludedOptions = normalizedLines(m_excludedOptions);
    const QStringList customPathLines = normalizedLines(m_customPaths);
    QList<SharedContent::CustomPath> customPaths;
    for (QString value : customPathLines) {
        const bool directory = value.endsWith('/');
        while (value.endsWith('/')) {
            value.chop(1);
        }
        if (!SharedContent::Manager::validateCustomPath(value, &error)) {
            showError(tr("save the custom paths"), error);
            return false;
        }
        customPaths.append({ value, directory });
    }

    const SharedContent::Categories oldCategories = manager->instanceCategories(m_instance);
    const QList<SharedContent::CustomPath> oldCustomPaths = manager->instanceCustomPaths(m_instance);

    bool customPathsChanged = oldCustomPaths.size() != customPaths.size();
    if (!customPathsChanged) {
        for (int i = 0; i < customPaths.size(); ++i) {
            if (oldCustomPaths.at(i).relativePath != customPaths.at(i).relativePath ||
                oldCustomPaths.at(i).directory != customPaths.at(i).directory) {
                customPathsChanged = true;
                break;
            }
        }
    }
    const bool sharingChanged = oldGroup != group || oldCategories != categories || customPathsChanged;
    if (sharingChanged && m_instance->isRunning()) {
        showError(tr("configure shared content"), tr("Stop the instance before changing its shared content."));
        return false;
    }
    SharedContent::MigrationPolicy policy = SharedContent::MigrationPolicy::PreferShared;

    if (sharingChanged) {
        QMessageBox box(QMessageBox::Question, tr("Reconcile Shared Content"),
                        tr("This instance and group “%1” may contain different files. Choose which version wins when the same path exists. "
                           "Prism Launcher keeps a backup while changing directory links.")
                            .arg(group),
                        QMessageBox::NoButton, this);
        auto* preferShared = box.addButton(tr("Prefer Shared"), QMessageBox::AcceptRole);
        auto* preferInstance = box.addButton(tr("Prefer This Instance"), QMessageBox::ActionRole);
        auto* cancel = box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(preferShared);
        box.exec();
        if (!box.clickedButton() || box.clickedButton() == cancel) {
            return false;
        }
        if (box.clickedButton() == preferInstance) {
            policy = SharedContent::MigrationPolicy::PreferInstance;
        }
    }

    if (!manager->configureInstance(m_instance, group, categories, customPaths, excludedOptions, policy, &error)) {
        loadSettings();
        showError(tr("configure shared content"), error);
        return false;
    }
    m_loadedGroup = group;
    return true;
}

void InstanceSharedContentPage::createGroup()
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
    m_enabled->setChecked(true);
}

void InstanceSharedContentPage::openGroup()
{
    const QString name = m_group->currentData().toString();
    if (name.isEmpty()) {
        return;
    }
    if (!DesktopServices::openPath(APPLICATION->sharedContent()->groupPath(name), true)) {
        showError(tr("open the shared group"), tr("The folder could not be opened."));
    }
}

void InstanceSharedContentPage::updateEnabledState()
{
    const bool enabled = m_enabled->isChecked();
    m_groupLabel->setEnabled(enabled);
    m_group->setEnabled(enabled);
    m_createGroupButton->setEnabled(enabled);
    m_openGroupButton->setEnabled(enabled && m_group->currentIndex() >= 0);
    m_categoriesBox->setEnabled(enabled);
    m_optionsBox->setEnabled(enabled);
    m_advancedBox->setEnabled(enabled);
}

void InstanceSharedContentPage::enabledToggled(bool enabled)
{
    if (enabled && !m_categorySelectionInitialized) {
        for (auto* check : m_categoryChecks) {
            check->setChecked(true);
        }
        m_categorySelectionInitialized = true;
    }
    updateEnabledState();
}

void InstanceSharedContentPage::showError(const QString& operation, const QString& error)
{
    QMessageBox::critical(this, tr("Shared Content Error"), tr("Could not %1:\n%2").arg(operation, error));
}
