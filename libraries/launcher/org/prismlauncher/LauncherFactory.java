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
 */

package org.prismlauncher;

import org.prismlauncher.impl.OneSixLauncher;
import org.prismlauncher.utils.Parameters;

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
