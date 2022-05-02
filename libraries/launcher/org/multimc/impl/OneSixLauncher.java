/* Copyright 2012-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.multimc.impl;

import org.multimc.Launcher;
import org.multimc.applet.LegacyFrame;
import org.multimc.utils.ParamBucket;
import org.multimc.utils.Utils;

import java.applet.Applet;
import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Collections;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

public final class OneSixLauncher implements Launcher {

    private static final int DEFAULT_WINDOW_WIDTH = 854;
    private static final int DEFAULT_WINDOW_HEIGHT = 480;

    private static final Logger LOGGER = Logger.getLogger("OneSixLauncher");

    // parameters, separated from ParamBucket
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

    private final ClassLoader classLoader;

    public OneSixLauncher(ParamBucket params) {
        classLoader = ClassLoader.getSystemClassLoader();

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

        String windowParams = params.firstSafe("windowParams", "854x480");

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
    }

    private void invokeMain(Class<?> mainClass) throws Exception {
        Method method = mainClass.getMethod("main", String[].class);

        method.invoke(null, (Object) mcParams.toArray(new String[0]));
    }

    private void legacyLaunch() throws Exception {
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

                LegacyFrame mcWindow = new LegacyFrame(windowTitle);

                mcWindow.start(
                        mcApplet,
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

    private void launchWithMainClass() throws Exception {
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
        if (traits.contains("legacyLaunch") || traits.contains("alphaLaunch")) {
            // legacy launch uses the applet wrapper
            legacyLaunch();
        } else {
            // normal launch just calls main()
            launchWithMainClass();
        }
    }

}
