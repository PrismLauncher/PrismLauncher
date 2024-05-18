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

package org.prismlauncher.legacy.utils.url;

import org.prismlauncher.utils.logging.Log;

import java.io.IOException;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.reflect.Method;
import java.net.Proxy;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;

/**
 * A utility class for URLs which uses reflection to access constructors for
 * internal classes.
 */
public final class UrlUtils {
    private static URLStreamHandler http;
    private static MethodHandle openConnection;

    static {
        try {
            // we first obtain the stock URLStreamHandler for http as we overwrite it later
            Method getURLStreamHandler = URL.class.getDeclaredMethod("getURLStreamHandler", String.class);
            getURLStreamHandler.setAccessible(true);
            http = (URLStreamHandler) getURLStreamHandler.invoke(null, "http");

            // we next find the openConnection method
            Method openConnectionReflect = URLStreamHandler.class.getDeclaredMethod("openConnection", URL.class, Proxy.class);
            openConnectionReflect.setAccessible(true);
            openConnection = MethodHandles.lookup().unreflect(openConnectionReflect);
        } catch (Throwable e) {
            Log.error("URL reflection failed - some features may not work", e);
        }
    }

    /**
     * Determines whether all the features of this class are available.
     *
     * @return <code>true</code> if all features can be used
     */
    public static boolean isSupported() {
        return http != null && openConnection != null;
    }

    public static URLConnection openConnection(URL url, Proxy proxy) throws IOException {
        if (http == null)
            throw new UnsupportedOperationException();

        if (url.getProtocol().equals("http"))
            return openConnection(http, url, proxy);

        // fall back to Java's default method
        // at this point, this should not cause a StackOverflowError unless we've missed
        // a protocol out from the if statements
        return url.openConnection();
    }

    public static URLConnection openConnection(URLStreamHandler handler, URL url, Proxy proxy) throws IOException {
        if (openConnection == null)
            throw new UnsupportedOperationException();

        try {
            return (URLConnection) openConnection.invokeExact(handler, url, proxy);
        } catch (IOException | Error | RuntimeException e) {
            throw e; // rethrow if possible
        } catch (Throwable e) {
            throw new AssertionError("openConnection should not throw", e); // oh dear! this isn't meant to happen
        }
    }
}
