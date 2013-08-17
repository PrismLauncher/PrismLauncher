// 
//  Copyright 2012 MultiMC Contributors
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

import java.applet.Applet;
import java.awt.Dimension;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Modifier;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipFile;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import org.simplericity.macify.eawt.Application;
import org.simplericity.macify.eawt.DefaultApplication;

public class MultiMCLauncher
{
	/**
	 * @param args
	 *            The arguments you want to launch Minecraft with. New path,
	 *            Username, Session ID.
	 */
	public static void main(String[] args)
	{
		if (args.length < 3)
		{
			System.out.println("Not enough arguments.");
			System.exit(-1);
		}
		
		// Set the OSX application icon first, if we are on OSX.
		Application application = new DefaultApplication();
		if(application.isMac())
		{
			try
			{
				BufferedImage image = ImageIO.read(new File("icon.png"));
				application.setApplicationIconImage(image);
			}
			catch (IOException e)
			{
				e.printStackTrace();
			}
		}
		
		String userName = args[0];
		String sessionId = args[1];
		String windowtitle = args[2];
		String windowParams = args[3];
		String lwjgl = args[4];
		String cwd = System.getProperty("user.dir");
		
		Dimension winSize = new Dimension(854, 480);
		boolean maximize = false;
		boolean compatMode = false;
		
		
		String[] dimStrings = windowParams.split("x");
		
		if (windowParams.equalsIgnoreCase("compatmode"))
		{
			compatMode = true;
		}
		else if (windowParams.equalsIgnoreCase("max"))
		{
			maximize = true;
		}
		else if (dimStrings.length == 2)
		{
			try
			{
				winSize = new Dimension(Integer.parseInt(dimStrings[0]),
						Integer.parseInt(dimStrings[1]));
			}
			catch (NumberFormatException e)
			{
				System.out.println("Invalid Window size argument, " +
						"using default.");
			}
		}
		else
		{
			System.out.println("Invalid Window size argument, " +
					"using default.");
		}
		
		try
		{
			File binDir = new File(cwd, "bin");
			File lwjglDir;
			if(lwjgl.equalsIgnoreCase("Mojang"))
				lwjglDir = binDir;
			else
				lwjglDir = new File(lwjgl);
			
			System.out.println("Loading jars...");
			String[] lwjglJars = new String[] {
				"lwjgl.jar", "lwjgl_util.jar", "jinput.jar"
			};
			
			URL[] urls = new URL[4];
			try
			{
				File f = new File(binDir, "minecraft.jar");
				urls[0] = f.toURI().toURL();
				System.out.println("Loading URL: " + urls[0].toString());

				for (int i = 1; i < urls.length; i++)
				{
					File jar = new File(lwjglDir, lwjglJars[i-1]);
					urls[i] = jar.toURI().toURL();
					System.out.println("Loading URL: " + urls[i].toString());
				}
			}
			catch (MalformedURLException e)
			{
				System.err.println("MalformedURLException, " + e.toString());
				System.exit(5);
			}
			
			System.out.println("Loading natives...");
			String nativesDir = new File(lwjglDir, "natives").toString();
			
			System.setProperty("org.lwjgl.librarypath", nativesDir);
			System.setProperty("net.java.games.input.librarypath", nativesDir);

			URLClassLoader cl = 
					new URLClassLoader(urls, MultiMCLauncher.class.getClassLoader());
			
			// Get the Minecraft Class.
			Class<?> mc = null;
			try
			{
				mc = cl.loadClass("net.minecraft.client.Minecraft");
				
				Field f = getMCPathField(mc);
				
				if (f == null)
				{
					System.err.println("Could not find Minecraft path field. Launch failed.");
					System.exit(-1);
				}
				
				f.setAccessible(true);
				f.set(null, new File(cwd));
				// And set it.
				System.out.println("Fixed Minecraft Path: Field was " + f.toString());
			}
			catch (ClassNotFoundException e)
			{
				System.err.println("Can't find main class. Searching...");
				
				// Look for any class that looks like the main class.
				File mcJar = new File(new File(cwd, "bin"), "minecraft.jar");
				ZipFile zip = null;
				try
				{
					zip = new ZipFile(mcJar);
				} catch (ZipException e1)
				{
					e1.printStackTrace();
					System.err.println("Search failed.");
					System.exit(-1);
				} catch (IOException e1)
				{
					e1.printStackTrace();
					System.err.println("Search failed.");
					System.exit(-1);
				}
				
				Enumeration<? extends ZipEntry> entries = zip.entries();
				ArrayList<String> classes = new ArrayList<String>();
				
				while (entries.hasMoreElements())
				{
					ZipEntry entry = entries.nextElement();
					if (entry.getName().endsWith(".class"))
					{
						String entryName = entry.getName().substring(0, entry.getName().lastIndexOf('.'));
						entryName = entryName.replace('/', '.');
						System.out.println("Found class: " + entryName);
						classes.add(entryName);
					}
				}
				
				for (String clsName : classes)
				{
					try
					{
						Class<?> cls = cl.loadClass(clsName);
						if (!Runnable.class.isAssignableFrom(cls))
						{
							continue;
						}
						else
						{
							System.out.println("Found class implementing runnable: " + 
									cls.getName());
						}
						
						if (getMCPathField(cls) == null)
						{
							continue;
						}
						else
						{
							System.out.println("Found class implementing runnable " +
									"with mcpath field: " + cls.getName());
						}
						
						mc = cls;
						break;
					}
					catch (ClassNotFoundException e1)
					{
						// Ignore
						continue;
					}
				}
				
				if (mc == null)
				{
					System.err.println("Failed to find Minecraft main class.");
					System.exit(-1);
				}
				else
				{
					System.out.println("Found main class: " + mc.getName());
				}
			}
			
			System.setProperty("minecraft.applet.TargetDirectory", cwd);
			
			String[] mcArgs = new String[2];
			mcArgs[0] = userName;
			mcArgs[1] = sessionId;
			
			if (compatMode)
			{
				System.out.println("Launching in compatibility mode...");
				mc.getMethod("main", String[].class).invoke(null, (Object) mcArgs);
			}
			else
			{
				System.out.println("Launching with applet wrapper...");
				try
				{
					Class<?> MCAppletClass = cl.loadClass(
							"net.minecraft.client.MinecraftApplet");
					Applet mcappl = (Applet) MCAppletClass.newInstance();
					MCFrame mcWindow = new MCFrame(windowtitle);
					mcWindow.start(mcappl, userName, sessionId, winSize, maximize);
				} catch (InstantiationException e)
				{
					System.out.println("Applet wrapper failed! Falling back " +
							"to compatibility mode.");
					mc.getMethod("main", String[].class).invoke(null, (Object) mcArgs);
				}
			}
		} catch (ClassNotFoundException e)
		{
			e.printStackTrace();
			System.exit(1);
		} catch (IllegalArgumentException e)
		{
			e.printStackTrace();
			System.exit(2);
		} catch (IllegalAccessException e)
		{
			e.printStackTrace();
			System.exit(2);
		} catch (InvocationTargetException e)
		{
			e.printStackTrace();
			System.exit(3);
		} catch (NoSuchMethodException e)
		{
			e.printStackTrace();
			System.exit(3);
		} catch (SecurityException e)
		{
			e.printStackTrace();
			System.exit(4);
		}
	}

	public static Field getMCPathField(Class<?> mc)
	{
		Field[] fields = mc.getDeclaredFields();
		
		for (int i = 0; i < fields.length; i++)
		{
			Field f = fields[i];
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
