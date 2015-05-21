/* Copyright 2012-2014 MultiMC Contributors
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

package org.multimc.onesix;

import org.multimc.*;

import java.applet.Applet;
import java.awt.*;
import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

public class OneSixLauncher implements Launcher
{
	// parameters, separated from ParamBucket
	private List<String> libraries;
	private List<String> extlibs;
	private List<String> mcparams;
	private List<String> mods;
	private List<String> jarmods;
	private List<String> coremods;
	private List<String> traits;
	private String appletClass;
	private String mainClass;
	private String natives;
	private String userName, sessionId;
	private String windowTitle;
	private String windowParams;

	// secondary parameters
	private Dimension winSize;
	private boolean maximize;
	private String cwd;

	// the much abused system classloader, for convenience (for further abuse)
	private MMCClassLoader cl;

	private void processParams(ParamBucket params) throws NotFoundException
	{
		libraries = params.all("cp");
		extlibs = params.all("ext");
		mcparams = params.allSafe("param", new ArrayList<String>());
		mainClass = params.firstSafe("mainClass", "net.minecraft.client.Minecraft");
		appletClass = params.firstSafe("appletClass", "net.minecraft.client.MinecraftApplet");
		mods = params.allSafe("mod", new ArrayList<String>());
		jarmods = params.allSafe("jarmod", new ArrayList<String>());
		coremods = params.allSafe("coremod", new ArrayList<String>());
		traits = params.allSafe("traits", new ArrayList<String>());
		natives = params.first("natives");

		userName = params.first("userName");
		sessionId = params.first("sessionId");
		windowTitle = params.firstSafe("windowTitle", "Minecraft");
		windowParams = params.firstSafe("windowParams", "854x480");

		cwd = System.getProperty("user.dir");
		winSize = new Dimension(854, 480);
		maximize = false;

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
			}
			catch (NumberFormatException ignored)
			{
			}
		}
	}

	private void printStats()
	{
		Utils.log("Main Class:");
		Utils.log("  " + mainClass);
		Utils.log();

		Utils.log("Native path:");
		Utils.log("  " + natives);
		Utils.log();

		Utils.log("Traits:");
		Utils.log("  " + traits);
		Utils.log();

		Utils.log("Libraries:");
		for (String s : libraries)
		{
			File f = new File(s);
			if (f.exists())
			{
				Utils.log("  " + s);
			}
			else
			{
				Utils.log("  " + s + " (missing)", "Warning");
			}
		}
		Utils.log();

		if (mods.size() > 0)
		{
			Utils.log("Mods:");
			for (String s : mods)
			{
				Utils.log("  " + s);
			}
			Utils.log();
		}

		if(coremods.size() > 0)
		{
			Utils.log("Core Mods:");
			for (String s : coremods)
			{
				Utils.log("  " + s);
			}
			Utils.log();
		}

		if(jarmods.size() > 0)
		{
			Utils.log("Jar Mods:");
			for (String s : jarmods)
			{
				Utils.log("  " + s);
			}
			Utils.log();
		}

		Utils.log("Params:");
		Utils.log("  " + mcparams.toString());
		Utils.log();
		if (maximize)
		{
			Utils.log("Window size: max (if available)");
		}
		else
		{
			Utils.log("Window size: " + Integer.toString(winSize.width) + " x " + Integer.toString(winSize.height));
		}
		Utils.log();
	}

	int legacyLaunch()
	{
		// Get the Minecraft Class and set the base folder
		Class<?> mc;
		try
		{
			mc = cl.loadClass(mainClass);

			Field f = Utils.getMCPathField(mc);

			if (f == null)
			{
				System.err.println("Could not find Minecraft path field.");
			}
			else
			{
				f.setAccessible(true);
				f.set(null, new File(cwd));
			}
		}
		catch (Exception e)
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
			Class<?> MCAppletClass = cl.loadClass(appletClass);
			Applet mcappl = (Applet)MCAppletClass.newInstance();
			LegacyFrame mcWindow = new LegacyFrame(windowTitle);
			mcWindow.start(mcappl, userName, sessionId, winSize, maximize);
		}
		catch (Exception e)
		{
			Utils.log("Applet wrapper failed:", "Error");
			e.printStackTrace(System.err);
			Utils.log();
			Utils.log("Falling back to compatibility mode.");
			try
			{
				mc.getMethod("main", String[].class).invoke(null, (Object)mcArgs);
			}
			catch (Exception e1)
			{
				Utils.log("Failed to invoke the Minecraft main class:", "Fatal");
				e1.printStackTrace(System.err);
				return -1;
			}
		}
		return 0;
	}

	int launchWithMainClass()
	{
		// window size, title and state, onesix
		if (maximize)
		{
			// FIXME: there is no good way to maximize the minecraft window in onesix.
			// the following often breaks linux screen setups
			// mcparams.add("--fullscreen");
		}
		else
		{
			mcparams.add("--width");
			mcparams.add(Integer.toString(winSize.width));
			mcparams.add("--height");
			mcparams.add(Integer.toString(winSize.height));
		}

		// Get the Minecraft Class.
		Class<?> mc;
		try
		{
			mc = cl.loadClass(mainClass);
		}
		catch (ClassNotFoundException e)
		{
			System.err.println("Failed to find Minecraft main class:");
			e.printStackTrace(System.err);
			return -1;
		}

		// get the main method.
		Method meth;
		try
		{
			meth = mc.getMethod("main", String[].class);
		}
		catch (NoSuchMethodException e)
		{
			System.err.println("Failed to acquire the main method:");
			e.printStackTrace(System.err);
			return -1;
		}

		// init params for the main method to chomp on.
		String[] paramsArray = mcparams.toArray(new String[mcparams.size()]);
		try
		{
			// static method doesn't have an instance
			meth.invoke(null, (Object)paramsArray);
		}
		catch (Exception e)
		{
			System.err.println("Failed to start Minecraft:");
			e.printStackTrace(System.err);
			return -1;
		}
		return 0;
	}

	@Override public int launch(ParamBucket params)
	{
		// get and process the launch script params
		try
		{
			processParams(params);
		}
		catch (NotFoundException e)
		{
			System.err.println("Not enough arguments.");
			e.printStackTrace(System.err);
			return -1;
		}

		// print the pretty things
		printStats();

		// extract native libs (depending on platform here... java!)
		Utils.log("Preparing native libraries...");
		String property = System.getProperty("os.arch");
		boolean is_64 =
			property.equalsIgnoreCase("x86_64") || property.equalsIgnoreCase("amd64");
		for (String extlib : extlibs)
		{
			try
			{
				String cleanlib = extlib.replace("${arch}", is_64 ? "64" : "32");
				File cleanlibf = new File(cleanlib);
				Utils.log("Extracting " + cleanlibf.getName());
				Utils.unzipNatives(cleanlibf, new File(natives));
			}
			catch (IOException e)
			{
				System.err.println("Failed to extract native library:");
				e.printStackTrace(System.err);
				return -1;
			}
		}
		Utils.log();

		try
		{
			cl = new MMCClassLoader(natives, libraries);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}

		final int[] result = {-1};

		// fix log4j by sticking it in a thread with custom contextclassloader
		Thread t = new Thread("main")
		{
			@Override public void run()
			{
				if (traits.contains("legacyLaunch") || traits.contains("alphaLaunch"))
				{
					// legacy launch uses the applet wrapper
					result[0] = legacyLaunch();
				}
				else
				{
					// normal launch just calls main()
					result[0] = launchWithMainClass();
				}
			}
		};
		t.setContextClassLoader(cl);
		t.start();
		try
		{
			t.join();
		}
		catch (InterruptedException e)
		{
			e.printStackTrace();
		}
		return result[0];
	}
}
