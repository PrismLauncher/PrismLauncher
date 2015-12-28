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
import java.util.regex.Matcher;
import java.util.regex.Pattern;


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
	@Deprecated
	public static void addLibraryPath(String pathToAdd) throws Exception
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

	/**
	 * Pushes bytes from in to out. Closes both streams no matter what.
	 * @param in the input stream
	 * @param out the output stream
	 * @throws IOException
	 */
	private static void copyStream(InputStream in, OutputStream out) throws IOException
	{
		try
		{
		byte[] buffer = new byte[4096];
		int len;

		while((len = in.read(buffer)) >= 0)
			out.write(buffer, 0, len);
		} finally
		{
			in.close();
			out.close();
		}
	}

	/**
	 * Replace a 'target' string 'suffix' with 'replacement'
	 */
	public static String replaceSuffix (String target, String suffix, String replacement)
	{
		if (!target.endsWith(suffix))
		{
			return target;
		}
		String prefix = target.substring(0, target.length() - suffix.length());
		return prefix + replacement;
	}

	/**
	 * Unzip zip file with natives 'source' into the folder 'targetFolder'
	 *
	 * Contains a hack for OSX. Yay.
	 * @param source
	 * @param targetFolder
	 * @throws IOException
	 */
	public static void unzipNatives(File source, File targetFolder) throws IOException
	{
		ZipFile zip = new ZipFile(source);

		boolean applyHacks = false;
		// find the first number in the version string, treat that as the major version
		String s = System.getProperty("java.version");
		Matcher matcher = Pattern.compile("\\d+").matcher(s);
		matcher.find();
		int major = Integer.valueOf(matcher.group());
		if (major >= 8)
		{
			applyHacks = true;
		}

		try
		{
			Enumeration entries = zip.entries();

			while (entries.hasMoreElements())
			{
				ZipEntry entry = (ZipEntry) entries.nextElement();

				String entryName = entry.getName();
				String fileName = entryName;
				if(applyHacks)
				{
					fileName = replaceSuffix(entryName, ".jnilib", ".dylib");
				}
				File targetFile = new File(targetFolder, fileName);
				if (targetFile.getParentFile() != null)
				{
					targetFile.getParentFile().mkdirs();
				}

				if (entry.isDirectory())
					continue;

				copyStream(zip.getInputStream(entry), new BufferedOutputStream(new FileOutputStream(targetFile)));
			}
		} finally
		{
			zip.close();
		}
	}
}

