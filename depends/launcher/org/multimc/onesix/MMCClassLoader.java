package org.multimc.onesix;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Arrays;
import java.util.List;

public class MMCClassLoader extends URLClassLoader
{
	public MMCClassLoader(String natives, List<String> allJars)
		throws MalformedURLException, ClassNotFoundException, NoSuchMethodException,
			   InvocationTargetException, IllegalAccessException, NoSuchFieldException
	{
		super(process(allJars));
		Method setProperty = loadClass("java.lang.System").getMethod("setProperty", String.class, String.class);
		setProperty.invoke(null, "java.library.path", natives);
		setProperty.invoke(null, "org.lwjgl.librarypath", natives);
		setProperty.invoke(null, "net.java.games.input.librarypath", natives);
	}

	private static URL[] process(List<String> allJars) throws MalformedURLException
	{
		URL[] urls = new URL[allJars.size()];
		for (int i = 0; i < allJars.size(); i++)
		{
			String jar = allJars.get(i);
			urls[i] = new File(jar).toURI().toURL();
		}
		return urls;
	}

	// TODO: use this method to use custom log configs
	//	@Override
	//	public URL findResource(String name)
	//	{
	//		return super.findResource(name);
	//	}
}
