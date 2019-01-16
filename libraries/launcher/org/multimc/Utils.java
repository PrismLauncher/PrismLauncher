/*
 * Copyright 2012-2019 MultiMC Contributors
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

package org.multimc;

import java.io.*;
import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class Utils
{
    /**
     * Combine two parts of a path.
     *
     * @param path1
     * @param path2
     * @return the paths, combined
     */
    public static String combine(String path1, String path2)
    {
        File file1 = new File(path1);
        File file2 = new File(file1, path2);
        return file2.getPath();
    }

    /**
     * Join a list of strings into a string using a separator!
     *
     * @param strings   the string list to join
     * @param separator the glue
     * @return the result.
     */
    public static String join(List<String> strings, String separator)
    {
        StringBuilder sb = new StringBuilder();
        String sep = "";
        for (String s : strings)
        {
            sb.append(sep).append(s);
            sep = separator;
        }
        return sb.toString();
    }

    /**
     * Finds a field that looks like a Minecraft base folder in a supplied class
     *
     * @param mc the class to scan
     */
    public static Field getMCPathField(Class<?> mc)
    {
        Field[] fields = mc.getDeclaredFields();

        for (Field f : fields)
        {
            if (f.getType() != File.class)
            {
                // Has to be File
                continue;
            }
            if (f.getModifiers() != (Modifier.PRIVATE + Modifier.STATIC))
            {
                // And Private Static.
                continue;
            }
            return f;
        }
        return null;
    }

    /**
     * Log to the MultiMC console
     *
     * @param message A String containing the message
     * @param level   A String containing the level name. See MinecraftLauncher::getLevel()
     */
    public static void log(String message, String level)
    {
        // Kinda dirty
        String tag = "!![" + level + "]!";
        System.out.println(tag + message.replace("\n", "\n" + tag));
    }

    public static void log(String message)
    {
        log(message, "MultiMC");
    }

    public static void log()
    {
        System.out.println();
    }
}

