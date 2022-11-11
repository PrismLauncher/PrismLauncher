// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

package org.prismlauncher.fix.skins;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.reflect.Method;
import java.net.Proxy;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;
import java.net.URLStreamHandlerFactory;

import org.prismlauncher.fix.Fix;
import org.prismlauncher.utils.Parameters;
import org.prismlauncher.utils.logging.Log;

public final class SkinFix implements Fix, URLStreamHandlerFactory {

    private static URLStreamHandler http;
    private static MethodHandle openConnection;
    private static MethodHandle openConnection2;

    static {
        try {
            Method getURLStreamHandler = URL.class.getDeclaredMethod("getURLStreamHandler", String.class);
            getURLStreamHandler.setAccessible(true);
            http = (URLStreamHandler) getURLStreamHandler.invoke(null, "http");

            Method openConnectionReflect = URLStreamHandler.class.getDeclaredMethod("openConnection", URL.class);
            openConnectionReflect.setAccessible(true);
            openConnection = MethodHandles.lookup().unreflect(openConnectionReflect);

            Method openConnectionReflect2 = URLStreamHandler.class.getDeclaredMethod("openConnection", URL.class,
                    Proxy.class);
            openConnectionReflect2.setAccessible(true);
            openConnection2 = MethodHandles.lookup().unreflect(openConnectionReflect2);
        } catch (Throwable e) {
            Log.error("Could not perform URL reflection; skin fix will not be availble", e);
        }
    }

    static URLConnection openConnection(URL url) throws Throwable {
        return (URLConnection) openConnection.invokeExact(http, url);
    }

    static URLConnection openConnection(URL url, Proxy proxy) throws Throwable {
        return (URLConnection) openConnection2.invokeExact(http, url, proxy);
    }

    @Override
    public String getName() {
        return "legacySkinFix";
    }

    @Override
    public boolean isApplicable(Parameters parameters) {
        return http != null && openConnection != null;
    }

    @Override
    public void apply() {
        URL.setURLStreamHandlerFactory(this);
    }

    @Override
    public URLStreamHandler createURLStreamHandler(String protocol) {
        if ("http".equals(protocol))
            return new SkinFixUrlStreamHandler();

        return null;
    }

}
