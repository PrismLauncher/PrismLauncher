// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

package org.polymc.impl;

import org.polymc.Launcher;
import org.polymc.applet.LegacyFrame;
import org.polymc.utils.Parameters;
import org.polymc.utils.Utils;

import java.applet.Applet;
import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Paths;
import java.util.Collections;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

public final class OneSixLauncher implements Launcher {

    private static final int DEFAULT_WINDOW_WIDTH = 854;
    private static final int DEFAULT_WINDOW_HEIGHT = 480;

    private static final Logger LOGGER = Logger.getLogger("OneSixLauncher");

    // parameters, separated from ParamBucket
    private final List<String> classPath;
    private final List<String> mcParams;
    private final List<String> traits;
    private final String appletClass;
    private final String mainClass;
    private final String userName, sessionId;
    private final String windowTitle;

    // secondary parameters
    private final int winSizeW;
    private final int winSizeH;
    private final boolean maximize;
    private final String cwd;

    private final String serverAddress;
    private final String serverPort;

    public OneSixLauncher(Parameters params) {
        classPath = params.allSafe("classPath", Collections.<String>emptyList());
        mcParams = params.allSafe("param", Collections.<String>emptyList());
        mainClass = params.firstSafe("mainClass", "net.minecraft.client.Minecraft");
        appletClass = params.firstSafe("appletClass", "net.minecraft.client.MinecraftApplet");
        traits = params.allSafe("traits", Collections.<String>emptyList());

        userName = params.first("userName");
        sessionId = params.first("sessionId");
        windowTitle = params.firstSafe("windowTitle", "Minecraft");

        serverAddress = params.firstSafe("serverAddress", null);
        serverPort = params.firstSafe("serverPort", null);

        cwd = System.getProperty("user.dir");

        String windowParams = params.firstSafe("windowParams", null);

        if (windowParams != null) {
            String[] dimStrings = windowParams.split("x");

            if (windowParams.equalsIgnoreCase("max")) {
                maximize = true;

                winSizeW = DEFAULT_WINDOW_WIDTH;
                winSizeH = DEFAULT_WINDOW_HEIGHT;
            } else if (dimStrings.length == 2) {
                maximize = false;

                winSizeW = Integer.parseInt(dimStrings[0]);
                winSizeH = Integer.parseInt(dimStrings[1]);
            } else {
                throw new IllegalArgumentException("Unexpected window size parameter value: " + windowParams);
            }
        } else {
            maximize = false;

            winSizeW = DEFAULT_WINDOW_WIDTH;
            winSizeH = DEFAULT_WINDOW_HEIGHT;
        }
    }

    private void invokeMain(Class<?> mainClass) throws Exception {
        Method method = mainClass.getMethod("main", String[].class);

        method.invoke(null, (Object) mcParams.toArray(new String[0]));
    }

    private void legacyLaunch(ClassLoader classLoader) throws Exception {
        // Get the Minecraft Class and set the base folder
        Class<?> minecraftClass = classLoader.loadClass(mainClass);

        Field baseDirField = Utils.getMinecraftBaseDirField(minecraftClass);

        if (baseDirField == null) {
            LOGGER.warning("Could not find Minecraft path field.");
        } else {
            baseDirField.setAccessible(true);

            baseDirField.set(null, new File(cwd));
        }

        System.setProperty("minecraft.applet.TargetDirectory", cwd);

        if (!traits.contains("noapplet")) {
            LOGGER.info("Launching with applet wrapper...");

            try {
                Class<?> mcAppletClass = classLoader.loadClass(appletClass);

                Applet mcApplet = (Applet) mcAppletClass.getConstructor().newInstance();

                LegacyFrame mcWindow = new LegacyFrame(windowTitle, mcApplet);

                mcWindow.start(
                        userName,
                        sessionId,
                        winSizeW,
                        winSizeH,
                        maximize,
                        serverAddress,
                        serverPort
                );

                return;
            } catch (Exception e) {
                LOGGER.log(Level.SEVERE, "Applet wrapper failed: ", e);

                LOGGER.warning("Falling back to using main class.");
            }
        }

        invokeMain(minecraftClass);
    }

    private void launchWithMainClass(ClassLoader classLoader) throws Exception {
        // window size, title and state, onesix

        // FIXME: there is no good way to maximize the minecraft window in onesix.
        // the following often breaks linux screen setups
        // mcparams.add("--fullscreen");

        if (!maximize) {
            mcParams.add("--width");
            mcParams.add(Integer.toString(winSizeW));
            mcParams.add("--height");
            mcParams.add(Integer.toString(winSizeH));
        }

        if (serverAddress != null) {
            mcParams.add("--server");
            mcParams.add(serverAddress);
            mcParams.add("--port");
            mcParams.add(serverPort);
        }

        invokeMain(classLoader.loadClass(mainClass));
    }

    @Override
    public void launch() throws Exception {
        URL[] classPathURLs = new URL[classPath.size()];
        for (int i = 0; i < classPath.size(); i++) {
            File f = new File(classPath.get(i));
            classPathURLs[i] = f.toURI().toURL();
        }
        // Some mod loaders (Fabric) read this property to determine the classpath.
        String systemClassPath = System.getProperty("java.class.path");
        systemClassPath += File.pathSeparator + String.join(File.pathSeparator, classPath);
        System.setProperty("java.class.path", systemClassPath);

        ClassLoader classLoader = new URLClassLoader(classPathURLs, getClass().getClassLoader());

        if (traits.contains("legacyLaunch") || traits.contains("alphaLaunch")) {
            // legacy launch uses the applet wrapper
            legacyLaunch(classLoader);
        } else {
            // normal launch just calls main()
            launchWithMainClass(classLoader);
        }
    }

}
