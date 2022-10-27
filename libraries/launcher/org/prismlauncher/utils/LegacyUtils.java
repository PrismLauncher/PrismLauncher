/*
 * Copyright 2012-2021 MultiMC Contributors
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

package org.prismlauncher.utils;


import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;


public final class LegacyUtils {
    
    private LegacyUtils() {
    }
    
    /**
     * Finds a field that looks like a Minecraft base folder in a supplied class
     *
     * @param clazz the class to scan
     */
    public static Field getMinecraftGameDirField(Class<?> clazz) {
        // Field we're looking for is always
        // private static File obfuscatedName = null;
        for (Field field : clazz.getDeclaredFields()) {
            // Has to be File
            if (field.getType() != File.class)
                continue;
            
            // And Private Static.
            if (!Modifier.isStatic(field.getModifiers()) || !Modifier.isPrivate(field.getModifiers()))
                continue;
            
            return field;
        }
        
        return null;
    }
    
}

