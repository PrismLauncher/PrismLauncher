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

package org.prismlauncher.launcher.impl;

import org.prismlauncher.applet.LegacyFrame;
import org.prismlauncher.utils.LegacyUtils;
import org.prismlauncher.utils.Parameters;

import java.applet.Applet;
import java.io.File;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.reflect.Field;
import java.util.Collections;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

@SuppressWarnings("removal")
public final class LegacyLauncher extends AbstractLauncher {

    private static final Logger LOGGER = Logger.getLogger("LegacyLauncher");

    private final String user, session;
    private final String title;
    private final String appletClass;

    private final boolean noApplet;
    private final String cwd;

    public LegacyLauncher(Parameters params) {
        super(params);

        user = params.getString("userName");
        session = params.getString("sessionId");
        title = params.getString("windowTitle", "Minecraft");
        appletClass = params.getString("appletClass", "net.minecraft.client.MinecraftApplet");

        List<String> traits = params.getList("traits", Collections.<String>emptyList());
        noApplet = traits.contains("noapplet");

        cwd = System.getProperty("user.dir");
    }

    @Override
    public void launch() throws Throwable {
        Class<?> main = loadMain();
        Field gameDirField = LegacyUtils.getMinecraftGameDirField(main);

        if (gameDirField == null) {
            LOGGER.warning("Could not find Mineraft path field.");
        } else {
            gameDirField.setAccessible(true);
            gameDirField.set(null, new File(cwd));
        }

        if (!noApplet) {
            LOGGER.info("Launching with applet wrapper...");

            try {
                Class<?> appletClass = classLoader.loadClass(this.appletClass);

                MethodHandle constructor = MethodHandles.lookup().findConstructor(appletClass, MethodType.methodType(void.class));
                Applet applet = (Applet) constructor.invoke();

                LegacyFrame window = new LegacyFrame(title, applet);

                window.start(
                        user,
                        session,
                        width,
                        height,
                        maximize,
                        serverAddress,
                        serverPort,
                        mcParams.contains("--demo")
                );

                return;
            } catch (Throwable e) {
                LOGGER.log(Level.SEVERE, "Applet wrapper failed:", e);

                LOGGER.warning("Falling back to using main class.");
            }
        }

        invokeMain(main);
    }

}
