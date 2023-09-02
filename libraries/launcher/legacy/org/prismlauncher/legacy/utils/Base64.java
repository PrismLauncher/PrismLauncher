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

package org.prismlauncher.legacy.utils;

import org.prismlauncher.utils.logging.Log;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.nio.charset.StandardCharsets;

/**
 * Uses Base64 with Java 8 or later, otherwise DatatypeConverter. In the latter
 * case, reflection is used to allow using newer compilers.
 */
public final class Base64 {
    private static boolean supported = true;
    private static MethodHandle legacy;

    static {
        try {
            Class.forName("java.util.Base64");
        } catch (ClassNotFoundException e) {
            try {
                Class<?> datatypeConverter = Class.forName("javax.xml.bind.DatatypeConverter");
                legacy = MethodHandles.lookup().findStatic(
                        datatypeConverter, "parseBase64Binary", MethodType.methodType(byte[].class, String.class));
            } catch (ClassNotFoundException | NoSuchMethodException | IllegalAccessException e1) {
                Log.error("Base64 not supported", e1);
                supported = false;
            }
        }
    }

    /**
     * Determines whether base64 is supported.
     *
     * @return <code>true</code> if base64 can be parsed
     */
    public static boolean isSupported() {
        return supported;
    }

    public static byte[] decode(String input) {
        if (!isSupported())
            throw new UnsupportedOperationException();

        if (legacy == null)
            return java.util.Base64.getDecoder().decode(input.getBytes(StandardCharsets.UTF_8));

        try {
            return (byte[]) legacy.invokeExact(input);
        } catch (Error | RuntimeException e) {
            throw e;
        } catch (Throwable e) {
            throw new Error(e);
        }
    }
}
