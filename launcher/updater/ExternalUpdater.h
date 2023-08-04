// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#ifndef LAUNCHER_EXTERNALUPDATER_H
#define LAUNCHER_EXTERNALUPDATER_H

#include <QObject>

/*!
 * A base class for an updater that uses an external library.
 * This class contains basic functions to control the updater.
 *
 * To implement the updater on a new platform, create a new class that inherits from this class and
 * implement the pure virtual functions.
 *
 * The initializer of the new class should have the side effect of starting the automatic updater. That is,
 * once the class is initialized, the program should automatically check for updates if necessary.
 */
class ExternalUpdater : public QObject {
    Q_OBJECT

   public:
    /*!
     * Check for updates manually, showing the user a progress bar and an alert if no updates are found.
     */
    virtual void checkForUpdates() = 0;

    /*!
     * Indicates whether or not to check for updates automatically.
     */
    virtual bool getAutomaticallyChecksForUpdates() = 0;

    /*!
     * Indicates the current automatic update check interval in seconds.
     */
    virtual double getUpdateCheckInterval() = 0;

    /*!
     * Indicates whether or not beta updates should be checked for in addition to regular releases.
     */
    virtual bool getBetaAllowed() = 0;

    /*!
     * Set whether or not to check for updates automatically.
     */
    virtual void setAutomaticallyChecksForUpdates(bool check) = 0;

    /*!
     * Set the current automatic update check interval in seconds.
     */
    virtual void setUpdateCheckInterval(double seconds) = 0;

    /*!
     * Set whether or not beta updates should be checked for in addition to regular releases.
     */
    virtual void setBetaAllowed(bool allowed) = 0;

   signals:
    /*!
     * Emits whenever the user's ability to check for updates changes.
     *
     * As per Sparkle documentation, "An update check can be made by the user when an update session isn’t in progress,
     * or when an update or its progress is being shown to the user. A user cannot check for updates when data (such
     * as the feed or an update) is still being downloaded automatically in the background.
     *
     * This property is suitable to use for menu item validation for seeing if checkForUpdates can be invoked."
     */
    void canCheckForUpdatesChanged(bool canCheck);
};

#endif  // LAUNCHER_EXTERNALUPDATER_H
