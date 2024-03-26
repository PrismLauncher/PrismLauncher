// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
import org.prismlauncher.launcher.impl.StandardLauncher;
import org.prismlauncher.legacy.LegacyProxy;
import org.prismlauncher.utils.Parameters;
import org.prismlauncher.utils.logging.Log;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public final class EntryPoint {
    public static void main(String[] args) {
        ExitCode code = listen();

        if (code != ExitCode.NORMAL) {
            Log.fatal("Exiting with " + code);

            System.exit(code.numeric);
        }
    }

    private static ExitCode listen() {
        Parameters params = new Parameters();
        PreLaunchAction action = PreLaunchAction.PROCEED;

        try (BufferedReader reader = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8))) {
            while (action == PreLaunchAction.PROCEED) {
                String line = reader.readLine();
                if (line != null)
                    action = parseLine(line, params);
                else
                    action = PreLaunchAction.ABORT;
            }
        } catch (IllegalArgumentException e) {
            Log.fatal("Aborting due to wrong argument", e);

            return ExitCode.ILLEGAL_ARGUMENT;
        } catch (Throwable e) {
            Log.fatal("Aborting due to exception", e);

            return ExitCode.ABORT;
        }

        if (action == PreLaunchAction.ABORT) {
            Log.fatal("Launch aborted by the launcher");

            return ExitCode.ABORT;
        }

        SystemProperties.apply(params);

        String launcherType = params.getString("launcher");

        try {
            LegacyProxy.applyOnlineFixes(params);

            Launcher launcher;

            switch (launcherType) {
                case "standard":
                    launcher = new StandardLauncher(params);
                    break;

                case "legacy":
                    launcher = LegacyProxy.createLauncher(params);
                    break;

                default:
                    throw new IllegalArgumentException("Invalid launcher type: " + launcherType);
            }

            launcher.launch();

            return ExitCode.NORMAL;
        } catch (IllegalArgumentException e) {
            Log.fatal("Illegal argument", e);

            return ExitCode.ILLEGAL_ARGUMENT;
        } catch (Throwable e) {
            Log.fatal("Exception caught from launcher", e);

            return ExitCode.ERROR;
        }
    }

    private static PreLaunchAction parseLine(String input, Parameters params) throws ParseException {
        switch (input) {
            case "":
                return PreLaunchAction.PROCEED;

            case "launch":
                return PreLaunchAction.LAUNCH;

            case "abort":
                return PreLaunchAction.ABORT;

            default:
                String[] pair = input.split(" ", 2);

                if (pair.length != 2)
                    throw new ParseException(input, "[key] [value]");

                params.add(pair[0], pair[1]);

                return PreLaunchAction.PROCEED;
        }
    }

    private enum PreLaunchAction { PROCEED, LAUNCH, ABORT }

    private enum ExitCode {
        NORMAL(0),
        ABORT(1),
        ERROR(2),
        ILLEGAL_ARGUMENT(65);

        private final int numeric;

        ExitCode(int numeric) {
            this.numeric = numeric;
        }
    }
}
