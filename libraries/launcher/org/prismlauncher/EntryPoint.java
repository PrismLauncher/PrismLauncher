// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 icelimetea <fr3shtea@outlook.com>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2022 solonovamax <solonovamax@12oclockpoint.com>
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
 *  Linking this library statically or dynamically with other modules is
 *  making a combined work based on this library. Thus, the terms and
 *  conditions of the GNU General Public License cover the whole
 *  combination.
 *
 *  As a special exception, the copyright holders of this library give
 *  you permission to link this library with independent modules to
 *  produce an executable, regardless of the license terms of these
 *  independent modules, and to copy and distribute the resulting
 *  executable under terms of your choice, provided that you also meet,
 *  for each linked independent module, the terms and conditions of the
 *  license of that module. An independent module is a module which is
 *  not derived from or based on this library. If you modify this
 *  library, you may extend this exception to your version of the
 *  library, but you are not obliged to do so. If you do not wish to do
 *  so, delete this exception statement from your version.
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

package org.prismlauncher;

import org.prismlauncher.exception.ParseException;
import org.prismlauncher.launcher.Launcher;
import org.prismlauncher.launcher.LauncherFactory;
import org.prismlauncher.utils.Parameters;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.logging.Level;
import java.util.logging.Logger;

public final class EntryPoint {

    private static final Logger LOGGER = Logger.getLogger("EntryPoint");

    public static void main(String[] args) {
        ExitCode exitCode = listen();

        if (exitCode != ExitCode.NORMAL) {
            LOGGER.warning("Exiting with " + exitCode);

            System.exit(exitCode.numericalCode);
        }
    }

    private static PreLaunchAction parseLine(String inData, Parameters params) throws ParseException {
        if (inData.isEmpty())
            throw new ParseException("Unexpected empty string!");

        String first = inData;
        String second = null;
        int splitPoint = inData.indexOf(' ');

        if (splitPoint != -1) {
            first = first.substring(0, splitPoint);
            second = inData.substring(splitPoint + 1);
        }

        switch (first) {
            case "launch":
                return PreLaunchAction.LAUNCH;
            case "abort":
                return PreLaunchAction.ABORT;
            default:
                if (second == null || second.isEmpty())
                    throw new ParseException("Error while parsing:" + inData);

                params.add(first, second);

                return PreLaunchAction.PROCEED;
        }
    }

    private static ExitCode listen() {
        Parameters parameters = new Parameters();
        PreLaunchAction preLaunchAction = PreLaunchAction.PROCEED;

        try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                System.in,
                StandardCharsets.UTF_8
        ))) {
            String line;

            while (preLaunchAction == PreLaunchAction.PROCEED) {
                if ((line = reader.readLine()) != null) {
                    preLaunchAction = parseLine(line, parameters);
                } else {
                    preLaunchAction = PreLaunchAction.ABORT;
                }
            }
        } catch (IOException | ParseException e) {
            LOGGER.log(Level.SEVERE, "Launcher abort due to exception:", e);

            return ExitCode.ERROR;
        }

        // Main loop
        if (preLaunchAction == PreLaunchAction.ABORT) {
            LOGGER.info("Launch aborted by the launcher.");

            return ExitCode.ERROR;
        }

        try {
            Launcher launcher = LauncherFactory.createLauncher(parameters);

            launcher.launch();

            return ExitCode.NORMAL;
        } catch (IllegalArgumentException e) {
            LOGGER.log(Level.SEVERE, "Wrong argument.", e);

            return ExitCode.ERROR;
        } catch (Throwable e) {
            LOGGER.log(Level.SEVERE, "Exception caught from launcher.", e);

            return ExitCode.ERROR;
        }
    }

    private enum PreLaunchAction {
        PROCEED,
        LAUNCH,
        ABORT
    }

    private enum ExitCode {
        NORMAL(0),
        ERROR(1);

        private final int numericalCode;

        ExitCode(int numericalCode) {
            this.numericalCode = numericalCode;
        }
    }

}
