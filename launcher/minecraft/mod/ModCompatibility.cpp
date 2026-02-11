// SPDX-FileCopyrightText: 2026 abhicommands <114682464+abhicommands@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 abhicommands <114682464+abhicommands@users.noreply.github.com>
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

#include "ModCompatibility.h"

#include <algorithm>

#include <QRegularExpression>

#include "Version.h"
#include "minecraft/mod/Resource.h"

namespace {

[[nodiscard]] QString normalizedVersionString(QString version)
{
    version = version.trimmed();
    if (version.startsWith("v", Qt::CaseInsensitive)) {
        version.remove(0, 1);
    }
    return version;
}

[[nodiscard]] bool startsWithVersionComparator(const QString& value)
{
    auto trimmedValue = value.trimmed();
    return trimmedValue.startsWith(">=") || trimmedValue.startsWith("<=") || trimmedValue.startsWith('>') || trimmedValue.startsWith('<') ||
           trimmedValue.startsWith('=');
}

[[nodiscard]] bool supportsWildcardPrefix(QString prefix, const QString& instanceMinecraftVersion)
{
    auto normalizedInstanceVersion = normalizedVersionString(instanceMinecraftVersion);
    prefix = normalizedVersionString(std::move(prefix));

    if (prefix.isEmpty() || normalizedInstanceVersion.isEmpty()) {
        return false;
    }

    if (normalizedInstanceVersion.compare(prefix, Qt::CaseInsensitive) == 0) {
        return true;
    }

    return normalizedInstanceVersion.startsWith(prefix + ".", Qt::CaseInsensitive);
}

[[nodiscard]] bool compareWithConstraint(const QString& instanceVersion, const QString& operation, const QString& versionConstraint)
{
    auto normalizedInstanceVersion = normalizedVersionString(instanceVersion);
    auto normalizedConstraint = normalizedVersionString(versionConstraint);

    if (normalizedInstanceVersion.isEmpty() || normalizedConstraint.isEmpty()) {
        return false;
    }

    Version instance{ normalizedInstanceVersion };
    Version constraint{ normalizedConstraint };

    if (operation == ">=") {
        return instance >= constraint;
    }
    if (operation == "<=") {
        return instance <= constraint;
    }
    if (operation == ">") {
        return instance > constraint;
    }
    if (operation == "<") {
        return instance < constraint;
    }
    if (operation == "=") {
        return instance == constraint;
    }

    return false;
}

[[nodiscard]] bool supportsSingleVersionPattern(const QString& pattern, const QString& instanceMinecraftVersion)
{
    auto trimmedPattern = pattern.trimmed();
    auto normalizedInstanceVersion = normalizedVersionString(instanceMinecraftVersion);
    if (trimmedPattern.isEmpty() || normalizedInstanceVersion.isEmpty()) {
        return false;
    }

    if (trimmedPattern == "*") {
        return true;
    }

    auto normalizedPattern = normalizedVersionString(trimmedPattern);
    if (normalizedPattern.compare(normalizedInstanceVersion, Qt::CaseInsensitive) == 0) {
        return true;
    }

    if (trimmedPattern.endsWith(".x", Qt::CaseInsensitive)) {
        auto prefix = trimmedPattern.left(trimmedPattern.size() - 2);
        return supportsWildcardPrefix(prefix, normalizedInstanceVersion);
    }

    if (trimmedPattern.endsWith(".*", Qt::CaseInsensitive)) {
        auto prefix = trimmedPattern.left(trimmedPattern.size() - 2);
        return supportsWildcardPrefix(prefix, normalizedInstanceVersion);
    }

    static const QRegularExpression s_comparatorPattern("^\\s*(<=|>=|<|>|=)\\s*(.+?)\\s*$");
    if (auto comparatorMatch = s_comparatorPattern.match(trimmedPattern); comparatorMatch.hasMatch()) {
        auto operation = comparatorMatch.captured(1);
        auto constraint = comparatorMatch.captured(2);
        return compareWithConstraint(normalizedInstanceVersion, operation, constraint);
    }

    static const QRegularExpression s_rangePattern("^\\s*(.+?)\\s+-\\s+(.+?)\\s*$");
    if (auto rangeMatch = s_rangePattern.match(trimmedPattern); rangeMatch.hasMatch()) {
        auto lowerBound = rangeMatch.captured(1);
        auto upperBound = rangeMatch.captured(2);
        return compareWithConstraint(normalizedInstanceVersion, ">=", lowerBound) &&
               compareWithConstraint(normalizedInstanceVersion, "<=", upperBound);
    }

    return false;
}

[[nodiscard]] bool supportsVersionPattern(const QString& pattern, const QString& instanceMinecraftVersion)
{
    auto trimmedPattern = pattern.trimmed();
    if (trimmedPattern.isEmpty()) {
        return false;
    }

    if (trimmedPattern.contains("||")) {
        for (auto const& alternative : trimmedPattern.split("||", Qt::SkipEmptyParts)) {
            if (supportsVersionPattern(alternative, instanceMinecraftVersion)) {
                return true;
            }
        }
        return false;
    }

    if (trimmedPattern.contains(',')) {
        auto alternatives = trimmedPattern.split(',', Qt::SkipEmptyParts);
        bool allConstraints = std::all_of(alternatives.cbegin(), alternatives.cend(),
                                          [](const QString& alternative) { return startsWithVersionComparator(alternative); });

        if (allConstraints) {
            for (auto const& constraint : alternatives) {
                if (!supportsSingleVersionPattern(constraint, instanceMinecraftVersion)) {
                    return false;
                }
            }
            return true;
        }

        for (auto const& alternative : alternatives) {
            if (supportsSingleVersionPattern(alternative, instanceMinecraftVersion)) {
                return true;
            }
        }
        return false;
    }

    auto whitespaceParts = trimmedPattern.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (whitespaceParts.size() > 1) {
        bool allConstraints = std::all_of(whitespaceParts.cbegin(), whitespaceParts.cend(),
                                          [](const QString& part) { return startsWithVersionComparator(part); });

        if (allConstraints) {
            for (auto const& constraint : whitespaceParts) {
                if (!supportsSingleVersionPattern(constraint, instanceMinecraftVersion)) {
                    return false;
                }
            }
            return true;
        }
    }

    return supportsSingleVersionPattern(trimmedPattern, instanceMinecraftVersion);
}

}  // namespace

namespace ModCompatibility {

bool supportsMinecraftVersion(const QStringList& supportedVersions, const QString& instanceMinecraftVersion)
{
    if (supportedVersions.isEmpty() || instanceMinecraftVersion.trimmed().isEmpty()) {
        return true;
    }

    for (auto const& supportedVersion : supportedVersions) {
        if (supportsVersionPattern(supportedVersion, instanceMinecraftVersion)) {
            return true;
        }
    }

    return false;
}

bool isIncompatibleWithInstanceVersion(const Resource& resource, const QString& instanceMinecraftVersion)
{
    if (instanceMinecraftVersion.isEmpty()) {
        return false;
    }

    if (!resource.enabled()) {
        return false;
    }

    auto metadata = resource.metadata();
    if (!metadata) {
        return false;
    }

    return !supportsMinecraftVersion(metadata->mcVersions, instanceMinecraftVersion);
}

}  // namespace ModCompatibility
