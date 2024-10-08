// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

package org.prismlauncher.legacy;

import org.prismlauncher.launcher.impl.AbstractLauncher;
import org.prismlauncher.utils.Parameters;
import org.prismlauncher.utils.ReflectionUtils;
import org.prismlauncher.utils.logging.Log;

import java.applet.Applet;
import java.io.File;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.Collections;
import java.util.List;

/**
 * Used to launch old versions which support applets.
 */
final class LegacyLauncher extends AbstractLauncher {
    private final String user, session;
    private final String title;
    private final String appletClass;
    private final boolean useApplet;
    private final String gameDir;

    public LegacyLauncher(Parameters params) {
        super(params);

        user = params.getString("userName");
        session = params.getString("sessionId");
        title = params.getString("windowTitle", "Minecraft");
        appletClass = params.getString("appletClass", "net.minecraft.client.MinecraftApplet");

        List<String> traits = params.getList("traits", Collections.<String>emptyList());
        useApplet = !traits.contains("noapplet");

        gameDir = System.getProperty("user.dir");
    }

    @Override
    public void launch() throws Throwable {
        Class<?> main = ClassLoader.getSystemClassLoader().loadClass(mainClassName);
        Field gameDirField = findMinecraftGameDirField(main);

        if (gameDirField != null) {
            gameDirField.setAccessible(true);
            gameDirField.set(null, new File(gameDir));
        }

        if (useApplet) {
            System.setProperty("minecraft.applet.TargetDirectory", gameDir);

            try {
                LegacyFrame window = new LegacyFrame(title, createAppletClass(appletClass));

                window.start(user, session, width, height, maximize, serverAddress, serverPort);
                return;
            } catch (Throwable e) {
                Log.error("Running applet wrapper failed with exception; falling back to main class", e);
            }
        }

        // find and invoke the main method, this time without size parameters - in all
        // versions that support applets, these are ignored
        MethodHandle method = ReflectionUtils.findMainMethod(main);
        method.invokeExact(gameArgs.toArray(new String[0]));
    }

    private static Applet createAppletClass(String clazz) throws Throwable {
        Class<?> appletClass = ClassLoader.getSystemClassLoader().loadClass(clazz);

        MethodHandle appletConstructor = MethodHandles.lookup().findConstructor(appletClass, MethodType.methodType(void.class));
        return (Applet) appletConstructor.invoke();
    }

    private static Field findMinecraftGameDirField(Class<?> clazz) {
        // search for private static File
        for (Field field : clazz.getDeclaredFields()) {
            if (field.getType() != File.class)
                continue;

            int fieldModifiers = field.getModifiers();

            if (!Modifier.isStatic(fieldModifiers))
                continue;

            if (!Modifier.isPrivate(fieldModifiers))
                continue;

            if (Modifier.isFinal(fieldModifiers))
                continue;

            return field;
        }

        return null;
    }
}
