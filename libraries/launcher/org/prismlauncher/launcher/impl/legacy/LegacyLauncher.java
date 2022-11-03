// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher
 *
 *  Copyright (C) 2022 icelimetea <fr3shtea@outlook.com>
 *  Copyright (C) 2022 flow <flowlnlnln@gmail.com>
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

package org.prismlauncher.launcher.impl.legacy;

import org.prismlauncher.launcher.Launcher;
import org.prismlauncher.launcher.LauncherProvider;
import org.prismlauncher.launcher.impl.AbstractLauncher;
import org.prismlauncher.utils.Parameters;
import org.prismlauncher.utils.ReflectionUtils;

import java.io.File;
import java.lang.invoke.MethodHandle;
import java.lang.reflect.Field;
import java.util.Collections;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Used to launch old versions that support applets.
 */
public final class LegacyLauncher extends AbstractLauncher {

    private static final Logger LOGGER = Logger.getLogger("LegacyLauncher");

    private final String user, session;
    private final String title;
    private final String appletClass;
    private final boolean usesApplet;
    private final String cwd;

    public LegacyLauncher(Parameters params) {
        super(params);

        user = params.getString("userName");
        session = params.getString("sessionId");
        title = params.getString("windowTitle", "Minecraft");
        appletClass = params.getString("appletClass", "net.minecraft.client.MinecraftApplet");

        List<String> traits = params.getList("traits", Collections.<String>emptyList());
        usesApplet = !traits.contains("noapplet");

        cwd = System.getProperty("user.dir");
    }

    public static LauncherProvider getProvider() {
        return new LegacyLauncherProvider();
    }

    @Override
    public void launch() throws Throwable {
        Class<?> main = ClassLoader.getSystemClassLoader().loadClass(this.mainClassName);
        Field gameDirField = ReflectionUtils.getMinecraftGameDirField(main);

        if (gameDirField == null)
            LOGGER.warning("Could not find Minecraft path field");
        else {
            gameDirField.setAccessible(true);
            gameDirField.set(null /* field is static, so instance is null */, new File(cwd));
        }

        if (this.usesApplet) {
            LOGGER.info("Launching legacy minecraft using applet wrapper...");

            try {
                LegacyFrame window = new LegacyFrame(title, ReflectionUtils.createAppletClass(this.appletClass));

                window.start(
                        this.user,
                        this.session,
                        this.width, this.height, this.maximize,
                        this.serverAddress, this.serverPort,
                        this.mcParams.contains("--demo")
                            );
            } catch (Throwable e) {
                LOGGER.log(Level.SEVERE, "Running applet wrapper failed with exception; falling back to main class", e);
            }
        }

        MethodHandle method = ReflectionUtils.findMainEntrypoint(main);
        method.invokeExact((Object[]) mcParams.toArray(new String[0]));
    }

    private static class LegacyLauncherProvider implements LauncherProvider {
        @Override
        public Launcher provide(Parameters parameters) {
            return new LegacyLauncher(parameters);
        }
    }

}
