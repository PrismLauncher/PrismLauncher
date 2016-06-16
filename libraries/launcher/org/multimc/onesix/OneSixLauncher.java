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
import java.io.File;
import java.awt.*;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

public class OneSixLauncher implements Launcher
{
	// parameters, separated from ParamBucket
	private List<String> libraries;
	private List<String> mcparams;
	private List<String> mods;
	private List<String> jarmods;
	private List<String> coremods;
	private List<String> traits;
	private String appletClass;
	private String mainClass;
	private String nativePath;
	private String userName, sessionId;
	private String windowTitle;
	private String windowParams;

	// secondary parameters
	private Dimension winSize;
	private boolean maximize;
	private String cwd;

	// the much abused system classloader, for convenience (for further abuse)
	private ClassLoader cl;

	private void processParams(ParamBucket params) throws NotFoundException
	{
		libraries = params.all("cp");
		mcparams = params.allSafe("param", new ArrayList<String>() );
		mainClass = params.firstSafe("mainClass", "net.minecraft.client.Minecraft");
		appletClass = params.firstSafe("appletClass", "net.minecraft.client.MinecraftApplet");
		traits = params.allSafe("traits", new ArrayList<String>());
		nativePath = params.first("natives");

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
			} catch (NumberFormatException ignored) {}
		}
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
			Class<?> MCAppletClass = cl.loadClass(appletClass);
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

		System.setProperty("minecraft.applet.TargetDirectory", cwd);

		// Get the Minecraft Class.
		Class<?> mc;
		try
		{
			mc = cl.loadClass(mainClass);
		} catch (ClassNotFoundException e)
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
		} catch (NoSuchMethodException e)
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
			meth.invoke(null, (Object) paramsArray);
		} catch (Exception e)
		{
			System.err.println("Failed to start Minecraft:");
			e.printStackTrace(System.err);
			return -1;
		}
		return 0;
	}

	@Override
	public int launch(ParamBucket params)
	{
		// get and process the launch script params
		try
		{
			processParams(params);
		} catch (NotFoundException e)
		{
			System.err.println("Not enough arguments.");
			e.printStackTrace(System.err);
			return -1;
		}

		// add libraries to classpath
		if(!Utils.addToClassPath(libraries))
		{
			System.err.println("Halting launch due to previous errors.");
			return -1;
		}

		// set the native libs path... the brute force way
		try
		{
			System.setProperty("java.library.path", nativePath);
			System.setProperty("org.lwjgl.librarypath", nativePath);
			System.setProperty("net.java.games.input.librarypath", nativePath);
			// by the power of reflection, initialize native libs again. DIRTY!
			// this is SO BAD. imagine doing that to ld
			Field fieldSysPath = ClassLoader.class.getDeclaredField("sys_paths");
			fieldSysPath.setAccessible( true );
			fieldSysPath.set( null, null );
		}
		catch (Exception e)
		{
			System.err.println("Failed to set the native library path:");
			e.printStackTrace(System.err);
			System.err.println("Minecraft might fail to launch...");
		}

		// grab the system classloader and ...
		cl = ClassLoader.getSystemClassLoader();

		if (traits.contains("legacyLaunch") || traits.contains("alphaLaunch") )
		{
			// legacy launch uses the applet wrapper
			return legacyLaunch();
		}
		else
		{
			// normal launch just calls main()
			return launchWithMainClass();
		}
	}
}
