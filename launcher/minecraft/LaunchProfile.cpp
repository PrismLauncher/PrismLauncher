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

#include "LaunchProfile.h"
#include <Version.h>

void LaunchProfile::clear()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersionType.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftAssets.reset();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftArguments.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_addnJvmArguments.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tweakers.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainClass.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_appletClass.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_libraries.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mavenFiles.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_agents.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_traits.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_jarMods.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainJar.reset();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problemSeverity = ProblemSeverity::None;
}

static void applyString(const QString & from, QString & to)
{
    if(from.isEmpty())
        return;
    to = from;
}

void LaunchProfile::applyMinecraftVersion(const QString& id)
{
    applyString(id, this->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion);
}

void LaunchProfile::applyAppletClass(const QString& appletClass)
{
    applyString(appletClass, this->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_appletClass);
}

void LaunchProfile::applyMainClass(const QString& mainClass)
{
    applyString(mainClass, this->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainClass);
}

void LaunchProfile::applyMinecraftArguments(const QString& minecraftArguments)
{
    applyString(minecraftArguments, this->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftArguments);
}

void LaunchProfile::applyAddnJvmArguments(const QStringList& addnJvmArguments)
{
    this->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_addnJvmArguments.append(addnJvmArguments);
}

void LaunchProfile::applyMinecraftVersionType(const QString& type)
{
    applyString(type, this->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersionType);
}

void LaunchProfile::applyMinecraftAssets(MojangAssetIndexInfo::Ptr assets)
{
    if(assets)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftAssets = assets;
    }
}

void LaunchProfile::applyTraits(const QSet<QString>& traits)
{
    this->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_traits.unite(traits);
}

void LaunchProfile::applyTweakers(const QStringList& tweakers)
{
    // if the applied tweakers override an existing one, skip it. this effectively moves it later in the sequence
    QStringList newTweakers;
    for(auto & tweaker: hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tweakers)
    {
        if (tweakers.contains(tweaker))
        {
            continue;
        }
        newTweakers.append(tweaker);
    }
    // then just append the new tweakers (or moved original ones)
    newTweakers += tweakers;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tweakers = newTweakers;
}

void LaunchProfile::applyJarMods(const QList<LibraryPtr>& jarMods)
{
    this->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_jarMods.append(jarMods);
}

static int findLibraryByName(QList<LibraryPtr> *haystack, const GradleSpecifier &needle)
{
    int retval = -1;
    for (int i = 0; i < haystack->size(); ++i)
    {
        if (haystack->at(i)->rawName().matchName(needle))
        {
            // only one is allowed.
            if (retval != -1)
                return -1;
            retval = i;
        }
    }
    return retval;
}

void LaunchProfile::applyMods(const QList<LibraryPtr>& mods)
{
    QList<LibraryPtr> * list = &hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods;
    for(auto & mod: mods)
    {
        auto modCopy = Library::limitedCopy(mod);

        // find the mod by name.
        const int index = findLibraryByName(list, mod->rawName());
        // mod not found? just add it.
        if (index < 0)
        {
            list->append(modCopy);
            return;
        }

        auto existingLibrary = list->at(index);
        // if we are higher it means we should update
        if (Version(mod->version()) > Version(existingLibrary->version()))
        {
            list->replace(index, modCopy);
        }
    }
}

void LaunchProfile::applyCompatibleJavaMajors(QList<int>& javaMajor)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_compatibleJavaMajors.append(javaMajor);
}

void LaunchProfile::applyLibrary(LibraryPtr library, const RuntimeContext & runtimeContext)
{
    if(!library->isActive(runtimeContext))
    {
        return;
    }

    QList<LibraryPtr> * list = &hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_libraries;
    if(library->isNative())
    {
        list = &hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_nativeLibraries;
    }

    auto libraryCopy = Library::limitedCopy(library);

    // find the library by name.
    const int index = findLibraryByName(list, library->rawName());
    // library not found? just add it.
    if (index < 0)
    {
        list->append(libraryCopy);
        return;
    }

    auto existingLibrary = list->at(index);
    // if we are higher it means we should update
    if (Version(library->version()) > Version(existingLibrary->version()))
    {
        list->replace(index, libraryCopy);
    }
}

void LaunchProfile::applyMavenFile(LibraryPtr mavenFile, const RuntimeContext & runtimeContext)
{
    if(!mavenFile->isActive(runtimeContext))
    {
        return;
    }

    if(mavenFile->isNative())
    {
        return;
    }

    // unlike libraries, we do not keep only one version or try to dedupe them
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mavenFiles.append(Library::limitedCopy(mavenFile));
}

void LaunchProfile::applyAgent(AgentPtr agent, const RuntimeContext & runtimeContext)
{
    auto lib = agent->library();
    if(!lib->isActive(runtimeContext))
    {
        return;
    }

    if(lib->isNative())
    {
        return;
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_agents.append(agent);
}

const LibraryPtr LaunchProfile::getMainJar() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainJar;
}

void LaunchProfile::applyMainJar(LibraryPtr jar)
{
    if(jar)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainJar = jar;
    }
}

void LaunchProfile::applyProblemSeverity(ProblemSeverity severity)
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problemSeverity < severity)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problemSeverity = severity;
    }
}

const QList<PatchProblem> LaunchProfile::getProblems() const
{
    // FIXME: implement something that actually makes sense here
    return {};
}

QString LaunchProfile::getMinecraftVersion() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion;
}

QString LaunchProfile::getAppletClass() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_appletClass;
}

QString LaunchProfile::getMainClass() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainClass;
}

const QSet<QString> &LaunchProfile::getTraits() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_traits;
}

const QStringList & LaunchProfile::getTweakers() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tweakers;
}

bool LaunchProfile::hasTrait(const QString& trait) const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_traits.contains(trait);
}

ProblemSeverity LaunchProfile::getProblemSeverity() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problemSeverity;
}

QString LaunchProfile::getMinecraftVersionType() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersionType;
}

std::shared_ptr<MojangAssetIndexInfo> LaunchProfile::getMinecraftAssets() const
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftAssets)
    {
        return std::make_shared<MojangAssetIndexInfo>("legacy");
    }
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftAssets;
}

QString LaunchProfile::getMinecraftArguments() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftArguments;
}

const QStringList & LaunchProfile::getAddnJvmArguments() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_addnJvmArguments;
}

const QList<LibraryPtr> & LaunchProfile::getJarMods() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_jarMods;
}

const QList<LibraryPtr> & LaunchProfile::getLibraries() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_libraries;
}

const QList<LibraryPtr> & LaunchProfile::getNativeLibraries() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_nativeLibraries;
}

const QList<LibraryPtr> & LaunchProfile::getMavenFiles() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mavenFiles;
}

const QList<AgentPtr> & LaunchProfile::getAgents() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_agents;
}

const QList<int> & LaunchProfile::getCompatibleJavaMajors() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_compatibleJavaMajors;
}

void LaunchProfile::getLibraryFiles(
    const RuntimeContext & runtimeContext,
    QStringList& jars,
    QStringList& nativeJars,
    const QString& overridePath,
    const QString& tempPath
) const
{
    QStringList native32, native64;
    jars.clear();
    nativeJars.clear();
    for (auto lib : getLibraries())
    {
        lib->getApplicableFiles(runtimeContext, jars, nativeJars, native32, native64, overridePath);
    }
    // NOTE: order is important here, add main jar last to the lists
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainJar)
    {
        // FIXME: HACK!! jar modding is weird and unsystematic!
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_jarMods.size())
        {
            QDir tempDir(tempPath);
            jars.append(tempDir.absoluteFilePath("minecraft.jar"));
        }
        else
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainJar->getApplicableFiles(runtimeContext, jars, nativeJars, native32, native64, overridePath);
        }
    }
    for (auto lib : getNativeLibraries())
    {
        lib->getApplicableFiles(runtimeContext, jars, nativeJars, native32, native64, overridePath);
    }
    if(runtimeContext.javaArchitecture == "32")
    {
        nativeJars.append(native32);
    }
    else if(runtimeContext.javaArchitecture == "64")
    {
        nativeJars.append(native64);
    }
}
