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

    public Launcher createLauncher(Parameters parameters) {
        String name = parameters.first("launcher");

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
