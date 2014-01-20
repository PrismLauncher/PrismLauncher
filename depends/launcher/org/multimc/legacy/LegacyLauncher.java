package org.multimc.legacy;/*
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

import org.multimc.Launcher;
import org.multimc.NotFoundException;
import org.multimc.ParamBucket;
import org.multimc.Utils;

import java.applet.Applet;
import java.awt.*;
import java.io.File;
import java.lang.reflect.Field;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;

public class LegacyLauncher implements Launcher
{
	@Override
	public int launch(ParamBucket params)
	{
		String userName, sessionId, windowTitle, windowParams, lwjgl;
		String mainClass = "net.minecraft.client.Minecraft";
		try
		{
			userName = params.first("userName");
			sessionId = params.first("sessionId");
			windowTitle = params.first("windowTitle");
			windowParams = params.first("windowParams");
			lwjgl = params.first("lwjgl");
		} catch (NotFoundException e)
		{
			System.err.println("Not enough arguments.");
			return -1;
		}

		String cwd = System.getProperty("user.dir");
		Dimension winSize = new Dimension(854, 480);
		boolean maximize = false;

		String[] dimStrings = windowParams.split("x");

		if (windowParams.equalsIgnoreCase("max"))
		{
			maximize = true;
		}
		else if (dimStrings.length == 2)
		{
			try
			{
				winSize = new Dimension(Integer.parseInt(dimStrings[0]), Integer.parseInt(dimStrings[1]));
			} catch (NumberFormatException ignored) {}
		}

		File binDir = new File(cwd, "bin");
		File lwjglDir;
		if (lwjgl.equalsIgnoreCase("Mojang"))
		{
			lwjglDir = binDir;
		}
		else
		{
			lwjglDir = new File(lwjgl);
		}

		URL[] classpath;
		{
			try
			{
				classpath = new URL[]
				{
					new File(binDir, "minecraft.jar").toURI().toURL(),
					new File(lwjglDir, "lwjgl.jar").toURI().toURL(),
					new File(lwjglDir, "lwjgl_util.jar").toURI().toURL(),
					new File(lwjglDir, "jinput.jar").toURI().toURL(),
				};
			} catch (MalformedURLException e)
			{
				System.err.println("Class path entry is badly formed:");
				e.printStackTrace(System.err);
				return -1;
			}
		}

		String nativesDir = new File(lwjglDir, "natives").toString();

		System.setProperty("org.lwjgl.librarypath", nativesDir);
		System.setProperty("net.java.games.input.librarypath", nativesDir);

		// print the pretty things
		{
			Utils.log("Main Class:");
			Utils.log("  " + mainClass);
			Utils.log();

			Utils.log("Class Path:");
			for (URL s : classpath)
			{
				Utils.log("  " + s);
			}
			Utils.log();

			Utils.log("Native Path:");
			Utils.log("  " + nativesDir);
			Utils.log();
		}

		URLClassLoader cl = new URLClassLoader(classpath, LegacyLauncher.class.getClassLoader());

		// Get the Minecraft Class and set the base folder
		Class<?> mc;
		try
		{
			mc = cl.loadClass(mainClass);

			Field f = Utils.getMCPathField(mc);

			if (f == null)
			{
				System.err.println("Could not find Minecraft path field. Launch failed.");
				return -1;
			}

			f.setAccessible(true);
			f.set(null, new File(cwd));
		} catch (Exception e)
		{
			System.err.println("Could not set base folder. Failed to find/access Minecraft main class:");
			e.printStackTrace(System.err);
			return -1;
		}

		System.setProperty("minecraft.applet.TargetDirectory", cwd);

		String[] mcArgs = new String[2];
		mcArgs[0] = userName;
		mcArgs[1] = sessionId;

		Utils.log("Launching with applet wrapper...");
		try
		{
			Class<?> MCAppletClass = cl.loadClass("net.minecraft.client.MinecraftApplet");
			Applet mcappl = (Applet) MCAppletClass.newInstance();
			LegacyFrame mcWindow = new LegacyFrame(windowTitle);
			mcWindow.start(mcappl, userName, sessionId, winSize, maximize);
		} catch (Exception e)
		{
			Utils.log("Applet wrapper failed:", "Error");
			e.printStackTrace(System.err);
			Utils.log();
			Utils.log("Falling back to compatibility mode.");
			try
			{
				mc.getMethod("main", String[].class).invoke(null, (Object) mcArgs);
			} catch (Exception e1)
			{
				Utils.log("Failed to invoke the Minecraft main class:", "Fatal");
				e1.printStackTrace(System.err);
				return -1;
			}
		}

		return 0;
	}
}
