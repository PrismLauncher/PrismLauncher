// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 icelimetea, <fr3shtea@outlook.com>
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

package org.multimc;

import org.multimc.impl.OneSixLauncher;
import org.multimc.utils.Parameters;

import java.util.HashMap;
import java.util.Map;

public final class LauncherFactory {

    private static final LauncherFactory INSTANCE = new LauncherFactory();

    private final Map<String, LauncherProvider> launcherRegistry = new HashMap<>();

    private LauncherFactory() {
        launcherRegistry.put("onesix", new LauncherProvider() {
            @Override
            public Launcher provide(Parameters parameters) {
                return new OneSixLauncher(parameters);
            }
        });
    }

    public Launcher createLauncher(String name, Parameters parameters) {
        LauncherProvider launcherProvider = launcherRegistry.get(name);

        if (launcherProvider == null)
            throw new IllegalArgumentException("Invalid launcher type: " + name);

        return launcherProvider.provide(parameters);
    }

    public static LauncherFactory getInstance() {
        return INSTANCE;
    }

    public interface LauncherProvider {

        Launcher provide(Parameters parameters);

    }

}
