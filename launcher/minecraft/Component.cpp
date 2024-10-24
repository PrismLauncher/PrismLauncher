// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "Component.h"
#include <meta/Index.h>
#include <meta/VersionList.h>

#include <QSaveFile>

#include "Application.h"
#include "FileSystem.h"
#include "OneSixVersionFormat.h"
#include "VersionFile.h"
#include "meta/Version.h"
#include "minecraft/Component.h"
#include "minecraft/PackProfile.h"

#include <assert.h>

const QMap<QString, ModloaderMapEntry> Component::KNOWN_MODLOADERS = {
    { "net.neoforged", { ModPlatform::NeoForge, { "net.minecraftforge", "net.fabricmc.fabric-loader", "org.quiltmc.quilt-loader" } } },
    { "net.minecraftforge", { ModPlatform::Forge, { "net.neoforged", "net.fabricmc.fabric-loader", "org.quiltmc.quilt-loader" } } },
    { "net.fabricmc.fabric-loader", { ModPlatform::Fabric, { "net.minecraftforge", "net.neoforged", "org.quiltmc.quilt-loader" } } },
    { "org.quiltmc.quilt-loader", { ModPlatform::Quilt, { "net.minecraftforge", "net.neoforged", "net.fabricmc.fabric-loader" } } },
    { "com.mumfrey.liteloader", { ModPlatform::LiteLoader, {} } }
};

Component::Component(PackProfile* parent, const QString& uid)
{
    assert(parent);
    m_parent = parent;

    m_uid = uid;
}

Component::Component(PackProfile* parent, const QString& uid, std::shared_ptr<VersionFile> file)
{
    assert(parent);
    m_parent = parent;

    m_file = file;
    m_uid = uid;
    m_cachedVersion = m_file->version;
    m_cachedName = m_file->name;
    m_loaded = true;
}

std::shared_ptr<Meta::Version> Component::getMeta()
{
    return m_metaVersion;
}

void Component::applyTo(LaunchProfile* profile)
{
    // do not apply disabled components
    if (!isEnabled()) {
        return;
    }
    auto vfile = getVersionFile();
    if (vfile) {
        vfile->applyTo(profile, m_parent->runtimeContext());
    } else {
        profile->applyProblemSeverity(getProblemSeverity());
    }
}

std::shared_ptr<class VersionFile> Component::getVersionFile() const
{
    if (m_metaVersion) {
        return m_metaVersion->data();
    } else {
        return m_file;
    }
}

std::shared_ptr<class Meta::VersionList> Component::getVersionList() const
{
    // FIXME: what if the metadata index isn't loaded yet?
    if (APPLICATION->metadataIndex()->hasUid(m_uid)) {
        return APPLICATION->metadataIndex()->get(m_uid);
    }
    return nullptr;
}

int Component::getOrder()
{
    if (m_orderOverride)
        return m_order;

    auto vfile = getVersionFile();
    if (vfile) {
        return vfile->order;
    }
    return 0;
}

void Component::setOrder(int order)
{
    m_orderOverride = true;
    m_order = order;
}

QString Component::getID()
{
    return m_uid;
}

QString Component::getName()
{
    if (!m_cachedName.isEmpty())
        return m_cachedName;
    return m_uid;
}

QString Component::getVersion()
{
    return m_cachedVersion;
}

QString Component::getFilename()
{
    return m_parent->patchFilePathForUid(m_uid);
}

QDateTime Component::getReleaseDateTime()
{
    if (m_metaVersion) {
        return m_metaVersion->time();
    }
    auto vfile = getVersionFile();
    if (vfile) {
        return vfile->releaseTime;
    }
    // FIXME: fake
    return QDateTime::currentDateTime();
}

bool Component::isEnabled()
{
    return !canBeDisabled() || !m_disabled;
}

bool Component::canBeDisabled()
{
    return isRemovable() && !m_dependencyOnly;
}

bool Component::setEnabled(bool state)
{
    bool intendedDisabled = !state;
    if (!canBeDisabled()) {
        intendedDisabled = false;
    }
    if (intendedDisabled != m_disabled) {
        m_disabled = intendedDisabled;
        emit dataChanged();
        return true;
    }
    return false;
}

bool Component::isCustom()
{
    return m_file != nullptr;
}

bool Component::isCustomizable()
{
    return m_metaVersion && getVersionFile();
}

bool Component::isRemovable()
{
    return !m_important;
}

bool Component::isRevertible()
{
    if (isCustom()) {
        if (APPLICATION->metadataIndex()->hasUid(m_uid)) {
            return true;
        }
    }
    return false;
}

bool Component::isMoveable()
{
    // HACK, FIXME: this was too dumb and wouldn't follow dependency constraints anyway. For now hardcoded to 'true'.
    return true;
}

bool Component::isVersionChangeable(bool wait)
{
    auto list = getVersionList();
    if (list) {
        if (wait)
            list->waitToLoad();
        return list->count() != 0;
    }
    return false;
}

bool Component::isKnownModloader()
{
    auto iter = KNOWN_MODLOADERS.find(m_uid);
    return iter != KNOWN_MODLOADERS.cend();
}

QStringList Component::knownConflictingComponents()
{
    auto iter = KNOWN_MODLOADERS.find(m_uid);
    if (iter != KNOWN_MODLOADERS.cend()) {
        return (*iter).knownConflictingComponents;
    } else {
        return {};
    }
}

void Component::setImportant(bool state)
{
    if (m_important != state) {
        m_important = state;
        emit dataChanged();
    }
}

ProblemSeverity Component::getProblemSeverity() const
{
    auto file = getVersionFile();
    if (file) {
        auto severity = file->getProblemSeverity();
        return m_componentProblemSeverity > severity ? m_componentProblemSeverity : severity;
    }
    return ProblemSeverity::Error;
}

const QList<PatchProblem> Component::getProblems() const
{
    auto file = getVersionFile();
    if (file) {
        auto problems = file->getProblems();
        problems.append(m_componentProblems);
        return problems;
    }
    return { { ProblemSeverity::Error, QObject::tr("Patch is not loaded yet.") } };
}

void Component::addComponentProblem(ProblemSeverity severity, const QString& description)
{
    if (severity > m_componentProblemSeverity) {
        m_componentProblemSeverity = severity;
    }
    m_componentProblems.append({ severity, description });

    emit dataChanged();
}

void Component::resetComponentProblems()
{
    m_componentProblems.clear();
    m_componentProblemSeverity = ProblemSeverity::None;

    emit dataChanged();
}

void Component::setVersion(const QString& version)
{
    if (version == m_version) {
        return;
    }
    m_version = version;
    if (m_loaded) {
        // we are loaded and potentially have state to invalidate
        if (m_file) {
            // we have a file... explicit version has been changed and there is nothing else to do.
        } else {
            // we don't have a file, therefore we are loaded with metadata
            m_cachedVersion = version;
            // see if the meta version is loaded
            auto metaVersion = APPLICATION->metadataIndex()->get(m_uid, version);
            if (metaVersion->isLoaded()) {
                // if yes, we can continue with that.
                m_metaVersion = metaVersion;
            } else {
                // if not, we need loading
                m_metaVersion.reset();
                m_loaded = false;
            }
            updateCachedData();
        }
    } else {
        // not loaded... assume it will be sorted out later by the update task
    }
    emit dataChanged();
}

bool Component::customize()
{
    if (isCustom()) {
        return false;
    }

    auto filename = getFilename();
    if (!FS::ensureFilePathExists(filename)) {
        return false;
    }
    // FIXME: get rid of this try-catch.
    try {
        QSaveFile jsonFile(filename);
        if (!jsonFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        auto vfile = getVersionFile();
        if (!vfile) {
            return false;
        }
        auto document = OneSixVersionFormat::versionFileToJson(vfile);
        jsonFile.write(document.toJson());
        if (!jsonFile.commit()) {
            return false;
        }
        m_file = vfile;
        m_metaVersion.reset();
        emit dataChanged();
    } catch (const Exception& error) {
        qWarning() << "Version could not be loaded:" << error.cause();
    }
    return true;
}

bool Component::revert()
{
    if (!isCustom()) {
        // already not custom
        return true;
    }
    auto filename = getFilename();
    bool result = true;
    // just kill the file and reload
    if (QFile::exists(filename)) {
        result = FS::deletePath(filename);
    }
    if (result) {
        // file gone...
        m_file.reset();

        // check local cache for metadata...
        auto version = APPLICATION->metadataIndex()->get(m_uid, m_version);
        if (version->isLoaded()) {
            m_metaVersion = version;
        } else {
            m_metaVersion.reset();
            m_loaded = false;
        }
        emit dataChanged();
    }
    return result;
}

/**
 * deep inspecting compare for requirement sets
 * By default, only uids are compared for set operations.
 * This compares all fields of the Require structs in the sets.
 */
static bool deepCompare(const std::set<Meta::Require>& a, const std::set<Meta::Require>& b)
{
    // NOTE: this needs to be rewritten if the type of Meta::RequireSet changes
    if (a.size() != b.size()) {
        return false;
    }
    for (const auto& reqA : a) {
        const auto& iter2 = b.find(reqA);
        if (iter2 == b.cend()) {
            return false;
        }
        const auto& reqB = *iter2;
        if (!reqA.deepEquals(reqB)) {
            return false;
        }
    }
    return true;
}

void Component::updateCachedData()
{
    auto file = getVersionFile();
    if (file) {
        bool changed = false;
        if (m_cachedName != file->name) {
            m_cachedName = file->name;
            changed = true;
        }
        if (m_cachedVersion != file->version) {
            m_cachedVersion = file->version;
            changed = true;
        }
        if (m_cachedVolatile != file->m_volatile) {
            m_cachedVolatile = file->m_volatile;
            changed = true;
        }
        if (!deepCompare(m_cachedRequires, file->m_requires)) {
            m_cachedRequires = file->m_requires;
            changed = true;
        }
        if (!deepCompare(m_cachedConflicts, file->conflicts)) {
            m_cachedConflicts = file->conflicts;
            changed = true;
        }
        if (changed) {
            emit dataChanged();
        }
    } else {
        // in case we removed all the metadata
        m_cachedRequires.clear();
        m_cachedConflicts.clear();
        emit dataChanged();
    }
}

void Component::waitLoadMeta()
{
    if (!m_loaded) {
        if (!m_metaVersion || !m_metaVersion->isLoaded()) {
            // wait for the loaded version from meta
            m_metaVersion = APPLICATION->metadataIndex()->getLoadedVersion(m_uid, m_version);
        }
        m_loaded = true;
        updateCachedData();
    }
}

void Component::setUpdateAction(UpdateAction action)
{
    m_updateAction = action;
}

UpdateAction Component::getUpdateAction()
{
    return m_updateAction;
}

void Component::clearUpdateAction()
{
    m_updateAction = UpdateAction{ UpdateActionNone{} };
}

QDebug operator<<(QDebug d, const Component& comp)
{
    d << "Component(" << comp.m_uid << " : " << comp.m_cachedVersion << ")";
    return d;
}
