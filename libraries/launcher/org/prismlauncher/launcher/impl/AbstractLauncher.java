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

import org.prismlauncher.exception.ParseException;
import org.prismlauncher.launcher.Launcher;
import org.prismlauncher.utils.Parameters;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.util.ArrayList;
import java.util.List;

public abstract class AbstractLauncher implements Launcher {

    private static final int DEFAULT_WINDOW_WIDTH = 854;
    private static final int DEFAULT_WINDOW_HEIGHT = 480;

    // parameters, separated from ParamBucket
    protected final List<String> mcParams;
    private final String mainClass;

    // secondary parameters
    protected final int width;
    protected final int height;
    protected final boolean maximize;

    protected final String serverAddress, serverPort;

    protected final ClassLoader classLoader;

    protected AbstractLauncher(Parameters params) {
        classLoader = ClassLoader.getSystemClassLoader();

        mcParams = params.getList("param", new ArrayList<String>());
        mainClass = params.getString("mainClass", "net.minecraft.client.Minecraft");

        serverAddress = params.getString("serverAddress", null);
        serverPort = params.getString("serverPort", null);

        String windowParams = params.getString("windowParams", null);

        if ("max".equals(windowParams) || windowParams == null) {
            maximize = windowParams != null;

            width = DEFAULT_WINDOW_WIDTH;
            height = DEFAULT_WINDOW_HEIGHT;
        } else {
            maximize = false;

            int byIndex = windowParams.indexOf('x');

            if (byIndex != -1) {
                try {
                    width = Integer.parseInt(windowParams.substring(0, byIndex));
                    height = Integer.parseInt(windowParams.substring(byIndex + 1));
                    return;
                } catch (NumberFormatException ignored) {
                }
            }

            throw new ParseException("Invalid window size parameter value: " + windowParams);
        }
    }

    protected Class<?> loadMain() throws ClassNotFoundException {
        return classLoader.loadClass(mainClass);
    }

    protected void loadAndInvokeMain() throws Throwable {
        invokeMain(loadMain());
    }

    protected void invokeMain(Class<?> mainClass) throws Throwable {
        MethodHandle method = MethodHandles.lookup().findStatic(mainClass, "main", MethodType.methodType(void.class, String[].class));

        method.invokeExact(mcParams.toArray(new String[0]));
    }

}
