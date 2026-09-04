// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Vivek Kushwaha <notvivekkushwaha@gmail.com>
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
 */

#include "ModFolderPage.h"
#include "ProfileCheckStateDelegate.h"
#include "ProfileOverviewWidget.h"
#include "minecraft/mod/Resource.h"
#include "ui/dialogs/ExportToModListDialog.h"
#include "ui/dialogs/InstallLoaderDialog.h"
#include "ui_ExternalResourcesPage.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHideEvent>
#include <QMenu>
#include <QShowEvent>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QInputDialog>
#include <QBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <algorithm>
#include <memory>

#include "Application.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/dialogs/ResourceUpdateDialog.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"

#include "tasks/ConcurrentTask.h"
#include "tasks/Task.h"
#include "ui/dialogs/ProgressDialog.h"

void ModFolderPage::toggleModMembership(const QString& modId, bool add)
{
    if (m_currentProfile.isEmpty() || modId.isEmpty())
        return;

    int row = -1;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        if (m_model->at(i).mod_id() == modId) {
            row = i;
            break;
        }
    }

    QSet<QString> idsToChange{ modId };
    if (row >= 0) {
        EnableAction action = add ? EnableAction::ENABLE : EnableAction::DISABLE;
        if (!confirmDependencyExpansion({ m_model->index(row, 0) }, action, idsToChange))
            return;
    }

    for (const QString& id : std::as_const(idsToChange)) {
        if (add)
            m_profileStates[m_currentProfile].insert(id);
        else
            m_profileStates[m_currentProfile].remove(id);
    }
    persistProfileState(m_currentProfile);
    repaintActiveColumn();
    if (!m_instance->isRunning()) {
        ++m_profileSwitchGeneration;
        auto op = beginOperation(m_currentProfile);
        syncDisplayedProfileToFilesystem(op);
    }
}

void ModFolderPage::persistProfileState(const QString& name)
{
    if (name.isEmpty())
        return;
    const QSet<QString>& state = m_profileStates.value(name);
    QString key = profileKey(name);
    m_instance->settings()->getOrRegisterSetting(key, QStringList());
    m_instance->settings()->set(key, QStringList(state.begin(), state.end()));
}

void ModFolderPage::setCurrentProfile(const QString& name)
{
    m_currentProfile = name;
    m_profileTabBar->setProperty("currentProfileName", name);
    m_instance->settings()->set(lastActiveNameKey(), name);
    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        if (m_profileTabBar->tabText(i) == name) {
            m_instance->settings()->set(lastActiveIndexKey(), i);
            break;
        }
    }
}

ProfileOperation ModFolderPage::beginOperation(const QString& profile)
{
    ProfileOperation op;
    op.generation = m_profileSwitchGeneration;
    op.profile    = profile;
    return op;
}

bool ModFolderPage::isCurrentOperation(const ProfileOperation& op) const
{
    return op.generation == m_profileSwitchGeneration && op.profile == m_currentProfile;
}

void ModFolderPage::syncDisplayedProfileToFilesystem(const ProfileOperation& op)
{
    if (m_instance->isRunning())
        return;
    if (m_currentProfile.isEmpty())
        return;
    QSet<QString> targetState = m_profileStates.value(m_currentProfile);
    m_applyingProfile = true;
    m_model->applyEnabledIds(targetState);
    connect(m_model, &ResourceFolderModel::updateFinished, this,
        [this, op] {
            if (isCurrentOperation(op)) {
                m_applyingProfile = false;
            }
        },
        Qt::SingleShotConnection);
    m_model->update();
}

bool ModFolderPage::confirmDependencyExpansion(const QModelIndexList& seedIndexes, EnableAction action, QSet<QString>& idsToChange)
{
    QModelIndexList affected = m_model->getAffectedMods(seedIndexes, action);
    if (affected.isEmpty())
        return true;

    const bool enabling = (action == EnableAction::ENABLE);
    QSet<QString> affectedIds;
    QString details = enabling
        ? tr("The following mods will be enabled:")
        : tr("The following mods will be disabled:");
    for (const auto& idx : affected) {
        const Mod& m = m_model->at(idx.row());
        affectedIds.insert(m.mod_id());
        details += QString("\n- %1 (%2)").arg(m.name(), m.internalId());
    }

    const QString title = tr("Confirm toggle");
    const QString noButton = tr("Only Toggle Selected");
    const QString yesButton = tr("Toggle Required Mods");

    QString message = tr("Toggling this mod will cause dependency changes to other mods in this profile.\n");
    message += enabling
        ? tr("%n mod(s) will be enabled\n", "", affected.size())
        : tr("%n mod(s) will be disabled\n", "", affected.size());
    message += tr("Do you want to automatically apply these related changes?\nIgnoring them may break the game.");

    auto* box = CustomMessageBox::selectable(this, title, message, QMessageBox::Warning,
                                             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
    box->button(QMessageBox::No)->setText(noButton);
    box->button(QMessageBox::Yes)->setText(yesButton);
    box->setDetailedText(details);

    const auto response = box->exec();

    if (response == QMessageBox::Cancel)
        return false;
    if (response == QMessageBox::Yes)
        idsToChange += affectedIds;
    return true;
}

void ModFolderPage::setAllModsInProfile(const QString& targetProfile, bool enableAll)
{
    if (targetProfile.isEmpty())
        return;
    QSet<QString> newState;
    if (enableAll) {
        for (int i = 0; i < m_model->rowCount(); ++i)
            newState.insert(m_model->at(i).mod_id());
    }
    m_profileStates[targetProfile] = newState;
    persistProfileState(targetProfile);
    repaintActiveColumn();
    if (!m_instance->isRunning()) {
        auto op = beginOperation(targetProfile);
        syncDisplayedProfileToFilesystem(op);
    }
}

void ModFolderPage::refreshOverview()
{
    if (!m_overviewWidget)
        return;
    QStringList profileNames;
    for (int i = 0; i < m_profileTabBar->count(); ++i)
        profileNames.append(m_profileTabBar->tabText(i));
    bool overviewDefault = loadOverviewDefault();
    QStringList selected = loadRuntimeSelection();
    m_overviewWidget->refresh(m_model, m_profileStates, profileNames,
                              overviewDefault, selected,
                              m_instance->isRunning(),
                              m_model->launchSnapshot());
    if (ui && ui->filterEdit)
        m_overviewWidget->filterTextChanged(ui->filterEdit->text());

    if (updateExtraInfo)
        updateExtraInfo(id(), extraHeaderInfoString());
}

bool ModFolderPage::loadOverviewDefault() const
{
    return m_instance->settings()->get(overviewDefaultKey()).toBool();
}

void ModFolderPage::persistOverviewSelection(bool defaultSelected, const QStringList& selectedProfiles)
{
    m_instance->settings()->set(overviewDefaultKey(), defaultSelected);
    saveRuntimeSelection(selectedProfiles);
}

void ModFolderPage::onModItemActivated(const QModelIndex& proxyIndex)
{
    if (!proxyIndex.isValid())
        return;
    QModelIndex sourceIndex = m_filterModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid())
        return;
    int row = sourceIndex.row();
    if (row < 0 || row >= m_model->rowCount())
        return;
    const QString modId = m_model->at(row).mod_id();
    if (modId.isEmpty())
        return;
    bool currentlyEnabled = m_profileStates.value(m_currentProfile).contains(modId);
    toggleModMembership(modId, !currentlyEnabled);
}

void ModFolderPage::onDelegateMembershipToggled(const QString& modId, bool enabled)
{
    toggleModMembership(modId, enabled);
}

void ModFolderPage::enableItem()
{
    if (m_currentProfile.isEmpty())
        return;
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    QModelIndexList seedIndexes;
    QSet<QString> idsToChange;
    for (const QModelIndex& idx : selection.indexes()) {
        if (idx.column() != 0)
            continue;
        int row = idx.row();
        if (row < 0 || row >= m_model->rowCount())
            continue;
        idsToChange.insert(m_model->at(row).mod_id());
        seedIndexes << idx;
    }
    if (idsToChange.isEmpty())
        return;

    if (!confirmDependencyExpansion(seedIndexes, EnableAction::ENABLE, idsToChange))
        return;

    bool changed = false;
    for (const QString& id : std::as_const(idsToChange)) {
        if (!m_profileStates[m_currentProfile].contains(id)) {
            m_profileStates[m_currentProfile].insert(id);
            changed = true;
        }
    }
    if (changed) {
        persistProfileState(m_currentProfile);
        repaintActiveColumn();
        if (!m_instance->isRunning()) {
            ++m_profileSwitchGeneration;
            auto op = beginOperation(m_currentProfile);
            syncDisplayedProfileToFilesystem(op);
        }
    }
}

void ModFolderPage::disableItem()
{
    if (m_currentProfile.isEmpty())
        return;
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    QModelIndexList seedIndexes;
    QSet<QString> idsToChange;
    for (const QModelIndex& idx : selection.indexes()) {
        if (idx.column() != 0)
            continue;
        int row = idx.row();
        if (row < 0 || row >= m_model->rowCount())
            continue;
        idsToChange.insert(m_model->at(row).mod_id());
        seedIndexes << idx;
    }
    if (idsToChange.isEmpty())
        return;

    if (!confirmDependencyExpansion(seedIndexes, EnableAction::DISABLE, idsToChange))
        return;

    bool changed = false;
    for (const QString& id : std::as_const(idsToChange)) {
        if (m_profileStates[m_currentProfile].contains(id)) {
            m_profileStates[m_currentProfile].remove(id);
            changed = true;
        }
    }
    if (changed) {
        persistProfileState(m_currentProfile);
        repaintActiveColumn();
        if (!m_instance->isRunning()) {
            ++m_profileSwitchGeneration;
            auto op = beginOperation(m_currentProfile);
            syncDisplayedProfileToFilesystem(op);
        }
    }
}

void ModFolderPage::onOverviewDefaultSelected()
{
    persistOverviewSelection(true, QStringList());
    refreshOverview();
}

void ModFolderPage::onOverviewProfileSelectionToggled(const QString& profileName, bool selected)
{
    QStringList current = loadRuntimeSelection();
    if (selected) {
        if (!current.contains(profileName))
            current.append(profileName);
        persistOverviewSelection(false, current);
    } else {
        current.removeAll(profileName);
        if (current.isEmpty())
            persistOverviewSelection(true, QStringList());
        else
            persistOverviewSelection(false, current);
    }
    refreshOverview();
}

void ModFolderPage::applyProfileSwitch(int index, int generation, bool isInitialLoad)
{
    if (!m_profileTabBar)
        return;
    if (isInitialLoad && m_instance->isRunning()) {
        return;
    }
    if (index >= 0 && index < m_profileTabBar->count()) {
        QString tabName = m_profileTabBar->tabText(index);
        setCurrentProfile(tabName);

        QSet<QString> enabledMods;
        if (m_profileStates.contains(tabName)) {
            enabledMods = m_profileStates[tabName];
        } else {
            QString key = profileKey(tabName);
            m_instance->settings()->getOrRegisterSetting(key, QStringList());
            QStringList saved = m_instance->settings()->get(key).toStringList();
            enabledMods = QSet<QString>(saved.begin(), saved.end());
            m_profileStates[tabName] = enabledMods;
        }

        if (!m_instance->isRunning()) {
            // On initial load, an absent profile key means the filesystem state remains authoritative.
            if (isInitialLoad && !m_instance->settings()->containsValue(profileKey(tabName))) {
                m_applyingProfile = false;
                repaintActiveColumn();
            } else {
                int capturedGeneration = generation;
                QString capturedProfile = tabName;
                m_applyingProfile = true;
                m_model->applyEnabledIds(enabledMods);
                connect(m_model, &ResourceFolderModel::updateFinished, this,
                    [this, capturedGeneration, capturedProfile] {
                        if (capturedGeneration != m_profileSwitchGeneration ||
                            capturedProfile != m_currentProfile) {
                            return;
                        }
                        m_applyingProfile = false;
                    },
                    Qt::SingleShotConnection);
                m_model->update();
            }
        } else {
            m_applyingProfile = false;
            repaintActiveColumn();
        }
    } else {
        m_currentProfile = QString();
        m_profileTabBar->setProperty("currentProfileName", QString());
        m_applyingProfile = false;
        repaintActiveColumn();
    }

    refreshOverviewIfActive();
}

void ModFolderPage::onAddProfileClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Profile"), tr("Profile Name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        createProfile(name, QSet<QString>{});
    }
}

void ModFolderPage::onRemoveProfileClicked() {
    int index = m_profileTabBar->currentIndex();
    if (index > 0) {
        onTabRemove(index);
    }
}

void ModFolderPage::createProfile(const QString& name, const QSet<QString>& initialState,
                                   int insertAfterIndex)
{
    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        if (m_profileTabBar->tabText(i) == name) {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("A profile with this name already exists."));
            return;
        }
    }
    m_profileStates[name] = initialState;
    QString key = profileKey(name);
    m_instance->settings()->getOrRegisterSetting(key, QStringList());
    m_instance->settings()->set(key, QStringList(initialState.begin(), initialState.end()));
    if (insertAfterIndex >= 0 && insertAfterIndex < m_profileTabBar->count()) {
        m_profileTabBar->insertTab(insertAfterIndex + 1, name);
    } else {
        m_profileTabBar->addTab(name);
    }
    m_profileTabBar->setUsesScrollButtons(m_profileTabBar->count() > 1);
    saveProfileList();
    refreshOverviewIfActive();
}

void ModFolderPage::saveProfileList()
{
    QStringList profileList;
    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        profileList.append(m_profileTabBar->tabText(i));
    }
    m_instance->settings()->set(profileListKey(), profileList);
}

void ModFolderPage::onTabContextMenuRequested(const QPoint& pos)
{
    int tabIndex = m_profileTabBar->tabAt(pos);
    QMenu menu(this);
    auto* newToRight = menu.addAction(tr("New Tab to Right"));
    int insertAfter = tabIndex >= 0 ? tabIndex : m_profileTabBar->count() - 1;
    connect(newToRight, &QAction::triggered, this, [this, insertAfter] {
        onTabNewToRight(insertAfter);
    });
    if (tabIndex >= 0) {
        auto* duplicate = menu.addAction(tr("Duplicate"));
        connect(duplicate, &QAction::triggered, this, [this, tabIndex] {
            onTabDuplicate(tabIndex);
        });
        if (tabIndex != 0) {
            auto* rename = menu.addAction(tr("Rename"));
            connect(rename, &QAction::triggered, this, [this, tabIndex] {
                onTabRename(tabIndex);
            });
            auto* remove = menu.addAction(tr("Remove"));
            connect(remove, &QAction::triggered, this, [this, tabIndex] {
                onTabRemove(tabIndex);
            });
        }
        menu.addSeparator();
        auto* enableAll = menu.addAction(tr("Enable All"));
        connect(enableAll, &QAction::triggered, this, [this, tabIndex] {
            onTabEnableAll(tabIndex);
        });
        auto* disableAll = menu.addAction(tr("Disable All"));
        connect(disableAll, &QAction::triggered, this, [this, tabIndex] {
            onTabDisableAll(tabIndex);
        });
    }
    menu.exec(m_profileTabBar->mapToGlobal(pos));
}

void ModFolderPage::onTabNewToRight(int sourceIndex)
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Profile"), tr("Profile Name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        createProfile(name, QSet<QString>{}, sourceIndex);
    }
}

void ModFolderPage::onTabDuplicate(int sourceIndex)
{
    if (m_applyingProfile) return;
    QString sourceName = m_profileTabBar->tabText(sourceIndex);
    QSet<QString> sourceState = m_profileStates.value(sourceName);
    bool ok;
    QString name = QInputDialog::getText(this, tr("Duplicate Profile"),
                                          tr("New Profile Name:"), QLineEdit::Normal,
                                          tr("Copy of %1").arg(sourceName), &ok);
    if (ok && !name.isEmpty()) {
        createProfile(name, sourceState, sourceIndex);
    }
}

void ModFolderPage::onTabRename(int tabIndex)
{
    if (tabIndex == 0) return;
    if (m_applyingProfile) return;
    QString oldName = m_profileTabBar->tabText(tabIndex);
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Profile"),
                                             tr("New Profile Name:"), QLineEdit::Normal,
                                             oldName, &ok);
    if (!ok || newName.isEmpty() || newName == oldName) return;
    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        if (m_profileTabBar->tabText(i) == newName) {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("A profile named \"%1\" already exists.").arg(newName));
            return;
        }
    }
    if (m_profileStates.contains(oldName)) {
        m_profileStates[newName] = m_profileStates.take(oldName);
    }
    QString oldKey = profileKey(oldName);
    QString newKey = profileKey(newName);
    QStringList stateData = m_instance->settings()->get(oldKey).toStringList();
    m_instance->settings()->reset(oldKey);
    m_instance->settings()->getOrRegisterSetting(newKey, QStringList());
    m_instance->settings()->set(newKey, stateData);
    m_profileTabBar->setTabText(tabIndex, newName);
    if (m_currentProfile == oldName) {
        setCurrentProfile(newName);
    }
    QStringList rtSelected = loadRuntimeSelection();
    int pos = rtSelected.indexOf(oldName);
    if (pos >= 0) {
        rtSelected[pos] = newName;
        saveRuntimeSelection(rtSelected);
    }
    saveProfileList();
    refreshOverviewIfActive();
}

void ModFolderPage::onTabRemove(int tabIndex)
{
    if (tabIndex == 0) return;
    if (m_applyingProfile) return;
    QString name = m_profileTabBar->tabText(tabIndex);
    auto response = CustomMessageBox::selectable(
                        this, tr("Remove Profile"),
                        tr("Are you sure you want to remove the profile \"%1\"?\n"
                           "This cannot be undone.").arg(name),
                        QMessageBox::Warning,
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No)->exec();
    if (response != QMessageBox::Yes) return;
    m_profileStates.remove(name);
    m_instance->settings()->reset(profileKey(name));
    QStringList runtimeSelected = loadRuntimeSelection();
    if (runtimeSelected.removeAll(name) > 0) {
        saveRuntimeSelection(runtimeSelected);
    }
    m_profileTabBar->removeTab(tabIndex);
    m_profileTabBar->setUsesScrollButtons(m_profileTabBar->count() > 1);
    if (m_currentProfile == name) {
        ++m_profileSwitchGeneration;
        applyProfileSwitch(0, m_profileSwitchGeneration);
    }
    saveProfileList();
    refreshOverviewIfActive();
}

void ModFolderPage::onTabEnableAll(int tabIndex)
{
    if (m_applyingProfile) {
        return;
    }
    QString targetProfile = m_profileTabBar->tabText(tabIndex);
    ++m_profileSwitchGeneration;
    if (m_profileTabBar->currentIndex() != tabIndex) {
        m_profileTabBar->blockSignals(true);
        m_profileTabBar->setCurrentIndex(tabIndex);
        m_profileTabBar->blockSignals(false);
    }
    setCurrentProfile(targetProfile);
    setAllModsInProfile(targetProfile, /*enableAll=*/true);
}

void ModFolderPage::onTabDisableAll(int tabIndex)
{
    if (m_applyingProfile) {
        return;
    }
    QString targetProfile = m_profileTabBar->tabText(tabIndex);
    ++m_profileSwitchGeneration;
    if (m_profileTabBar->currentIndex() != tabIndex) {
        m_profileTabBar->blockSignals(true);
        m_profileTabBar->setCurrentIndex(tabIndex);
        m_profileTabBar->blockSignals(false);
    }
    setCurrentProfile(targetProfile);
    setAllModsInProfile(targetProfile, /*enableAll=*/false);
}

QStringList ModFolderPage::loadRuntimeSelection() const
{
    return m_instance->settings()->get(runtimeProfilesKey()).toStringList();
}

void ModFolderPage::saveRuntimeSelection(const QStringList& selected)
{
    m_instance->settings()->set(runtimeProfilesKey(), selected);
}
