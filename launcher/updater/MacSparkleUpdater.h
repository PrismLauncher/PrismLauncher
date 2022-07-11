// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Kenneth Chew <kenneth.c0@protonmail.com>
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

#ifndef LAUNCHER_MACSPARKLEUPDATER_H
#define LAUNCHER_MACSPARKLEUPDATER_H

#include <QObject>
#include <QSet>
#include "ExternalUpdater.h"

/*!
 * An implementation for the updater on macOS that uses the Sparkle framework.
 */
class MacSparkleUpdater : public ExternalUpdater
{
    Q_OBJECT

public:
    /*!
     * Start the Sparkle updater, which automatically checks for updates if necessary.
     */
    MacSparkleUpdater();
    ~MacSparkleUpdater() override;

    /*!
     * Check for updates manually, showing the user a progress bar and an alert if no updates are found.
     */
    void checkForUpdates() override;

    /*!
     * Indicates whether or not to check for updates automatically.
     */
    bool getAutomaticallyChecksForUpdates() override;

    /*!
     * Indicates the current automatic update check interval in seconds.
     */
    double getUpdateCheckInterval() override;

    /*!
     * Indicates the set of Sparkle channels the updater is allowed to find new updates from.
     */
    QSet<QString> getAllowedChannels();

    /*!
     * Indicates whether or not beta updates should be checked for in addition to regular releases.
     */
    bool getBetaAllowed() override;

    /*!
     * Set whether or not to check for updates automatically.
     *
     * As per Sparkle documentation, "By default, Sparkle asks users on second launch for permission if they want
     * automatic update checks enabled and sets this property based on their response. If SUEnableAutomaticChecks is
     * set in the Info.plist, this permission request is not performed however.
     *
     * Setting this property will persist in the host bundle’s user defaults. Only set this property if you need
     * dynamic behavior (e.g. user preferences).
     *
     * The update schedule cycle will be reset in a short delay after the property’s new value is set. This is to allow
     * reverting this property without kicking off a schedule change immediately."
     */
    void setAutomaticallyChecksForUpdates(bool check) override;

    /*!
     * Set the current automatic update check interval in seconds.
     *
     * As per Sparkle documentation, "Setting this property will persist in the host bundle’s user defaults. For this
     * reason, only set this property if you need dynamic behavior (eg user preferences). Otherwise prefer to set
     * SUScheduledCheckInterval directly in your Info.plist.
     *
     * The update schedule cycle will be reset in a short delay after the property’s new value is set. This is to allow
     * reverting this property without kicking off a schedule change immediately."
     */
    void setUpdateCheckInterval(double seconds) override;

    /*!
     * Clears all allowed Sparkle channels, returning to the default updater channel behavior.
     */
    void clearAllowedChannels();

    /*!
     * Set a single Sparkle channel the updater is allowed to find new updates from.
     *
     * Items in the default channel can always be found, regardless of this setting. If an empty string is passed,
     * return to the default behavior.
     */
    void setAllowedChannel(const QString& channel);

    /*!
     * Set a set of Sparkle channels the updater is allowed to find new updates from.
     *
     * Items in the default channel can always be found, regardless of this setting. If an empty set is passed,
     * return to the default behavior.
     */
    void setAllowedChannels(const QSet<QString>& channels);

    /*!
     * Set whether or not beta updates should be checked for in addition to regular releases.
     */
    void setBetaAllowed(bool allowed) override;

private:
    class Private;

    Private *priv;

    void loadChannelsFromSettings();
};

#endif //LAUNCHER_MACSPARKLEUPDATER_H
