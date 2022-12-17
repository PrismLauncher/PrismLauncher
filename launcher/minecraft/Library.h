// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#pragma once
#include <QString>
#include <net/NetAction.h>
#include <QPair>
#include <QList>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDir>
#include <QUrl>
#include <memory>

#include "Rule.h"
#include "GradleSpecifier.h"
#include "MojangDownloadInfo.h"
#include "RuntimeContext.h"

class Library;
class MinecraftInstance;

typedef std::shared_ptr<Library> LibraryPtr;

class Library
{
    friend class OneSixVersionFormat;
    friend class MojangVersionFormat;
    friend class LibraryTest;
public:
    Library()
    {
    }
    Library(const QString &name)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name = name;
    }
    /// limited copy without some data. TODO: why?
    static LibraryPtr limitedCopy(LibraryPtr base)
    {
        auto newlib = std::make_shared<Library>();
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_repositoryURL = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_repositoryURL;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hint = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hint;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_absoluteURL = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_absoluteURL;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractExcludes = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractExcludes;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_nativeClassifiers = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_nativeClassifiers;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rules = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rules;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_storagePrefix = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_storagePrefix;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mojangDownloads = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mojangDownloads;
        newlib->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename = base->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename;
        return newlib;
    }

public: /* methods */
    /// Returns the raw name field
    const GradleSpecifier & rawName() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name;
    }

    void setRawName(const GradleSpecifier & spec)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name = spec;
    }

    void setClassifier(const QString & spec)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name.setClassifier(spec);
    }

    /// returns the full group and artifact prefix
    QString artifactPrefix() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name.artifactPrefix();
    }

    /// get the artifact ID
    QString artifactId() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name.artifactId();
    }

    /// get the artifact version
    QString version() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name.version();
    }

    /// Returns true if the library is native
    bool isNative() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_nativeClassifiers.size() != 0;
    }

    void setStoragePrefix(QString prefix = QString());

    /// Set the url base for downloads
    void setRepositoryURL(const QString &base_url)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_repositoryURL = base_url;
    }

    void getApplicableFiles(const RuntimeContext & runtimeContext, QStringList & jar, QStringList & native,
                            QStringList & native32, QStringList & native64, const QString & overridePath) const;

    void setAbsoluteUrl(const QString &absolute_url)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_absoluteURL = absolute_url;
    }

    void setFilename(const QString &filename)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename = filename;
    }

    /// Get the file name of the library
    QString filename(const RuntimeContext & runtimeContext) const;

    // DEPRECATED: set a display name, used by jar mods only
    void setDisplayName(const QString & displayName)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_displayname = displayName;
    }

    /// Get the file name of the library
    QString displayName(const RuntimeContext & runtimeContext) const;

    void setMojangDownloadInfo(MojangLibraryDownloadInfo::Ptr info)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mojangDownloads = info;
    }

    void setHint(const QString &hint)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hint = hint;
    }

    /// Set the load rules
    void setRules(QList<std::shared_ptr<Rule>> rules)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rules = rules;
    }

    /// Returns true if the library should be loaded (or extracted, in case of natives)
    bool isActive(const RuntimeContext & runtimeContext) const;

    /// Returns true if the library is contained in an instance and false if it is shared
    bool isLocal() const;

    /// Returns true if the library is to always be checked for updates
    bool isAlwaysStale() const;

    /// Return true if the library requires forge XZ hacks
    bool isForge() const;

    // Get a list of downloads for this library
    QList<NetAction::Ptr> getDownloads(const RuntimeContext & runtimeContext, class HttpMetaCache * cache,
                                     QStringList & failedLocalFiles, const QString & overridePath) const;

    QString getCompatibleNative(const RuntimeContext & runtimeContext) const;

private: /* methods */
    /// the default storage prefix used by Prism Launcher
    static QString defaultStoragePrefix();

    /// Get the prefix - root of the storage to be used
    QString storagePrefix() const;

    /// Get the relative file path where the library should be saved
    QString storageSuffix(const RuntimeContext & runtimeContext) const;

    QString hint() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hint;
    }

protected: /* data */
    /// the basic gradle dependency specifier.
    GradleSpecifier hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name;

    /// DEPRECATED URL prefix of the maven repo where the file can be downloaded
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_repositoryURL;

    /// DEPRECATED: Prism Launcher-specific absolute URL. takes precedence over the implicit maven repo URL, if defined
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_absoluteURL;

    /// Prism Launcher extension - filename override
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename;

    /// DEPRECATED Prism Launcher extension - display name
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_displayname;

    /**
     * Prism Launcher-specific type hint - modifies how the library is treated
     */
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hint;

    /**
     * storage - by default the local libraries folder in Prism Launcher, but could be elsewhere
     * Prism Launcher specific, because of FTB.
     */
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_storagePrefix;

    /// true if the library had an extract/excludes section (even empty)
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hasExcludes = false;

    /// a list of files that shouldn't be extracted from the library
    QStringList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractExcludes;

    /// native suffixes per OS
    QMap<QString, QString> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_nativeClassifiers;

    /// true if the library had a rules section (even empty)
    bool applyRules = false;

    /// rules associated with the library
    QList<std::shared_ptr<Rule>> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rules;

    /// MOJANG: container with Mojang style download info
    MojangLibraryDownloadInfo::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mojangDownloads;
};

