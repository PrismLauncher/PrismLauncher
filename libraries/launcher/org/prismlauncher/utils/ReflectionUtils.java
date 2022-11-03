// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher
 *
 *  Copyright (C) 2022 solonovamax <solonovamax@12oclockpoint.com>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2022 icelimetea <fr3shtea@outlook.com>
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

package org.prismlauncher.utils;

import java.applet.Applet;
import java.io.File;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.logging.Level;
import java.util.logging.Logger;

public final class ReflectionUtils {

    private static final Logger LOGGER = Logger.getLogger("ReflectionUtils");

    private ReflectionUtils() {
    }

    /**
     * Instantiate an applet class by name
     *
     * @param appletClassName The name of the applet class to resolve
     *
     * @return The instantiated applet class
     *
     * @throws ClassNotFoundException if the provided class name cannot be found
     * @throws NoSuchMethodException  if the no-args constructor cannot be found
     * @throws IllegalAccessException if the constructor cannot be accessed via
     *                                method handles
     * @throws Throwable              any exceptions from the class's constructor
     */
    public static Applet createAppletClass(String appletClassName) throws Throwable {
        Class<?> appletClass = ClassLoader.getSystemClassLoader().loadClass(appletClassName);

        MethodHandle appletConstructor = MethodHandles.lookup().findConstructor(appletClass,
                MethodType.methodType(void.class));
        return (Applet) appletConstructor.invoke();
    }

    /**
     * Finds a field that looks like a Minecraft base folder in a supplied class
     *
     * @param minecraftMainClass the class to scan
     *
     * @return The found field.
     */
    public static Field getMinecraftGameDirField(Class<?> minecraftMainClass) {
        LOGGER.fine("Resolving minecraft game directory field");
        // Field we're looking for is always
        // private static File obfuscatedName = null;
        for (Field field : minecraftMainClass.getDeclaredFields()) {
            // Has to be File
            if (field.getType() != File.class) {
                continue;
            }

            int fieldModifiers = field.getModifiers();

            // Must be static
            if (!Modifier.isStatic(fieldModifiers)) {
                LOGGER.log(Level.FINE, "Rejecting field {0} because it is not static", field.getName());
                continue;
            }

            // Must be private
            if (!Modifier.isPrivate(fieldModifiers)) {
                LOGGER.log(Level.FINE, "Rejecting field {0} because it is not private", field.getName());
                continue;
            }

            // Must not be final
            if (Modifier.isFinal(fieldModifiers)) {
                LOGGER.log(Level.FINE, "Rejecting field {0} because it is final", field.getName());
                continue;
            }

            LOGGER.log(Level.FINE, "Identified field {0} to match conditions for minecraft game directory field",
                    field.getName());

            return field;
        }

        return null;
    }

    /**
     * Resolve main entrypoint and returns method handle for it.
     * <p>
     * Resolves a method that matches the following signature <code>
     * public static void main(String[] args) {
     * <p>
     * }
     * </code>
     *
     * @param entrypointClass The entrypoint class to resolve the method from
     *
     * @return The method handle for the resolved entrypoint
     *
     * @throws NoSuchMethodException  If no method matching the correct signature
     *                                can be found
     * @throws IllegalAccessException If method handles cannot access the entrypoint
     */
    public static MethodHandle findMainEntrypoint(Class<?> entrypointClass)
            throws NoSuchMethodException, IllegalAccessException {
        return MethodHandles.lookup().findStatic(entrypointClass, "main",
                MethodType.methodType(void.class, String[].class));
    }

    /**
     * Resolve main entrypoint and returns method handle for it.
     * <p>
     * Resolves a method that matches the following signature <code>
     * public static void main(String[] args) {
     * <p>
     * }
     * </code>
     *
     * @param entrypointClassName The name of the entrypoint class to resolve the
     *                            method from
     *
     * @return The method handle for the resolved entrypoint
     *
     * @throws ClassNotFoundException If a class cannot be found with the provided
     *                                name
     * @throws NoSuchMethodException  If no method matching the correct signature
     *                                can be found
     * @throws IllegalAccessException If method handles cannot access the entrypoint
     */
    public static MethodHandle findMainMethod(String entrypointClassName)
            throws ClassNotFoundException, NoSuchMethodException, IllegalAccessException {
        return findMainEntrypoint(ClassLoader.getSystemClassLoader().loadClass(entrypointClassName));
    }

}
