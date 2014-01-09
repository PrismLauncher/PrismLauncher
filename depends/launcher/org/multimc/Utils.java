/*
 * Copyright 2012-2014 MultiMC Contributors
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

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Arrays;
import java.util.List;

public class Utils
{
	/**
	 * Adds the specified library to the classpath
	 *
	 * @param s the path to add
	 * @throws Exception
	 */
	public static void addToClassPath(String s) throws Exception
	{
		File f = new File(s);
		URL u = f.toURI().toURL();
		URLClassLoader urlClassLoader = (URLClassLoader) ClassLoader.getSystemClassLoader();
		Class urlClass = URLClassLoader.class;
		Method method = urlClass.getDeclaredMethod("addURL", new Class[]{URL.class});
		method.setAccessible(true);
		method.invoke(urlClassLoader, new Object[]{u});
	}

	/**
	 * Adds many libraries to the classpath
	 *
	 * @param jars the paths to add
	 */
	public static boolean addToClassPath(List<String> jars)
	{
		boolean pure = true;
		// initialize the class path
		for (String jar : jars)
		{
			try
			{
				Utils.addToClassPath(jar);
			} catch (Exception e)
			{
				System.err.println("Unable to load: " + jar);
				e.printStackTrace(System.err);
				pure = false;
			}
		}
		return pure;
	}

	/**
	 * Adds the specified path to the java library path
	 *
	 * @param pathToAdd the path to add
	 * @throws Exception
	 */
	@Deprecated public static void addLibraryPath(String pathToAdd) throws Exception
	{
		final Field usrPathsField = ClassLoader.class.getDeclaredField("usr_paths");
		usrPathsField.setAccessible(true);

		//get array of paths
		final String[] paths = (String[]) usrPathsField.get(null);

		//check if the path to add is already present
		for (String path : paths)
		{
			if (path.equals(pathToAdd))
			{
				return;
			}
		}

		//add the new path
		final String[] newPaths = Arrays.copyOf(paths, paths.length + 1);
		newPaths[newPaths.length - 1] = pathToAdd;
		usrPathsField.set(null, newPaths);
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
}
