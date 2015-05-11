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

public class Utils
{
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
		String[] javaVersionElements = System.getProperty("java.version").split("\\.");
		int major = Integer.parseInt(javaVersionElements[1]);
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

