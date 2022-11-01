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
     * @throws IllegalAccessException if the constructor cannot be accessed via method handles
     * @throws Throwable              any exceptions from the class's constructor
     */
    public static Applet createAppletClass(String appletClassName) throws Throwable {
        Class<?> appletClass = ClassLoader.getSystemClassLoader().loadClass(appletClassName);

        MethodHandle appletConstructor = MethodHandles.lookup().findConstructor(appletClass, MethodType.methodType(void.class));
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

            LOGGER.log(Level.FINE, "Identified field {0} to match conditions for minecraft game directory field", field.getName());

            return field;
        }

        return null;
    }

    /**
     * Resolve main entrypoint and returns method handle for it.
     * <p>
     * Resolves a method that matches the following signature
     * <code>
     * public static void main(String[] args) {
     * <p>
     * }
     * </code>
     *
     * @param entrypointClass The entrypoint class to resolve the method from
     *
     * @return The method handle for the resolved entrypoint
     *
     * @throws NoSuchMethodException  If no method matching the correct signature can be found
     * @throws IllegalAccessException If method handles cannot access the entrypoint
     */
    public static MethodHandle findMainEntrypoint(Class<?> entrypointClass) throws NoSuchMethodException, IllegalAccessException {
        return MethodHandles.lookup().findStatic(entrypointClass, "main", MethodType.methodType(void.class, String[].class));
    }

    /**
     * Resolve main entrypoint and returns method handle for it.
     * <p>
     * Resolves a method that matches the following signature
     * <code>
     * public static void main(String[] args) {
     * <p>
     * }
     * </code>
     *
     * @param entrypointClassName The name of the entrypoint class to resolve the method from
     *
     * @return The method handle for the resolved entrypoint
     *
     * @throws ClassNotFoundException If a class cannot be found with the provided name
     * @throws NoSuchMethodException  If no method matching the correct signature can be found
     * @throws IllegalAccessException If method handles cannot access the entrypoint
     */
    public static MethodHandle findMainEntrypoint(String entrypointClassName) throws
                                                                              ClassNotFoundException,
                                                                              NoSuchMethodException,
                                                                              IllegalAccessException {
        return findMainEntrypoint(ClassLoader.getSystemClassLoader().loadClass(entrypointClassName));
    }
}
