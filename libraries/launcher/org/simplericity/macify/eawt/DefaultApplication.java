package org.simplericity.macify.eawt;

/*
 * Copyright 2007 Eirik Bjorsnos.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.*;
import java.lang.reflect.*;
import java.net.URL;
import java.net.URLClassLoader;
import java.net.MalformedURLException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;


/**
 * Implements Application by calling the Mac OS X API through reflection.
 * If this class is used on a non-OS X platform the operations will have no effect or they will simulate
 * what the Apple API would do for those who manipulate state. ({@link #setEnabledAboutMenu(boolean)} etc.)
 */
@SuppressWarnings("unchecked")
public class DefaultApplication implements Application {

    private Object application;
    private Class applicationListenerClass;

    Map listenerMap = Collections.synchronizedMap(new HashMap<Object, Object>());
    private boolean enabledAboutMenu = true;
    private boolean enabledPreferencesMenu;
    private boolean aboutMenuItemPresent = true;
    private boolean preferencesMenuItemPresent;
    private ClassLoader classLoader;

    public DefaultApplication() {
        try {
            final File file = new File("/System/Library/Java");
            if (file.exists()) {
                ClassLoader scl = ClassLoader.getSystemClassLoader();
                Class clc = scl.getClass();
                if (URLClassLoader.class.isAssignableFrom(clc)) {
                    Method addUrl = URLClassLoader.class.getDeclaredMethod("addURL", new Class[]{URL.class});
                    addUrl.setAccessible(true);
                    addUrl.invoke(scl, new Object[]{file.toURI().toURL()});
                }
            }

            Class appClass = Class.forName("com.apple.eawt.Application");
            application = appClass.getMethod("getApplication", new Class[0]).invoke(null, new Object[0]);
            applicationListenerClass = Class.forName("com.apple.eawt.ApplicationListener");
        } catch (ClassNotFoundException e) {
            application = null;
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        } catch (MalformedURLException e) {
            throw new RuntimeException(e);
        }

    }

    public boolean isMac() {
        return application != null;
    }

    public void addAboutMenuItem() {
        if (isMac()) {
            callMethod(application, "addAboutMenuItem");
        } else {
            this.aboutMenuItemPresent = true;
        }
    }

    public void addApplicationListener(ApplicationListener applicationListener) {

        if (!Modifier.isPublic(applicationListener.getClass().getModifiers())) {
            throw new IllegalArgumentException("ApplicationListener must be a public class");
        }
        if (isMac()) {
            Object listener = Proxy.newProxyInstance(getClass().getClassLoader(),
                    new Class[]{applicationListenerClass},
                    new ApplicationListenerInvocationHandler(applicationListener));

            callMethod(application, "addApplicationListener", new Class[]{applicationListenerClass}, new Object[]{listener});
            listenerMap.put(applicationListener, listener);
        } else {
            listenerMap.put(applicationListener, applicationListener);
        }
    }

    public void addPreferencesMenuItem() {
        if (isMac()) {
            callMethod("addPreferencesMenuItem");
        } else {
            this.preferencesMenuItemPresent = true;
        }
    }

    public boolean getEnabledAboutMenu() {
        if (isMac()) {
            return callMethod("getEnabledAboutMenu").equals(Boolean.TRUE);
        } else {
            return enabledAboutMenu;
        }
    }

    public boolean getEnabledPreferencesMenu() {
        if (isMac()) {
            Object result = callMethod("getEnabledPreferencesMenu");
            return result.equals(Boolean.TRUE);
        } else {
            return enabledPreferencesMenu;
        }
    }

    public Point getMouseLocationOnScreen() {
        if (isMac()) {
            try {
                Method method = application.getClass().getMethod("getMouseLocationOnScreen", new Class[0]);
                return (Point) method.invoke(null, new Object[0]);
            } catch (NoSuchMethodException e) {
                throw new RuntimeException(e);
            } catch (IllegalAccessException e) {
                throw new RuntimeException(e);
            } catch (InvocationTargetException e) {
                throw new RuntimeException(e);
            }
        } else {
            return new Point(0, 0);
        }
    }

    public boolean isAboutMenuItemPresent() {
        if (isMac()) {
            return callMethod("isAboutMenuItemPresent").equals(Boolean.TRUE);
        } else {
            return aboutMenuItemPresent;
        }
    }

    public boolean isPreferencesMenuItemPresent() {
        if (isMac()) {
            return callMethod("isPreferencesMenuItemPresent").equals(Boolean.TRUE);
        } else {
            return this.preferencesMenuItemPresent;
        }
    }

    public void removeAboutMenuItem() {
        if (isMac()) {
            callMethod("removeAboutMenuItem");
        } else {
            this.aboutMenuItemPresent = false;
        }
    }

    public synchronized void removeApplicationListener(ApplicationListener applicationListener) {
        if (isMac()) {
            Object listener = listenerMap.get(applicationListener);
            callMethod(application, "removeApplicationListener", new Class[]{applicationListenerClass}, new Object[]{listener});

        }
        listenerMap.remove(applicationListener);
    }

    public void removePreferencesMenuItem() {
        if (isMac()) {
            callMethod("removeAboutMenuItem");
        } else {
            this.preferencesMenuItemPresent = false;
        }
    }

    public void setEnabledAboutMenu(boolean enabled) {
        if (isMac()) {
            callMethod(application, "setEnabledAboutMenu", new Class[]{Boolean.TYPE}, new Object[]{Boolean.valueOf(enabled)});
        } else {
            this.enabledAboutMenu = enabled;
        }
    }

    public void setEnabledPreferencesMenu(boolean enabled) {
        if (isMac()) {
            callMethod(application, "setEnabledPreferencesMenu", new Class[]{Boolean.TYPE}, new Object[]{Boolean.valueOf(enabled)});
        } else {
            this.enabledPreferencesMenu = enabled;
        }

    }

    public int requestUserAttention(int type) {
        if (type != REQUEST_USER_ATTENTION_TYPE_CRITICAL && type != REQUEST_USER_ATTENTION_TYPE_INFORMATIONAL) {
            throw new IllegalArgumentException("Requested user attention type is not allowed: " + type);
        }
        try {
            Object application = getNSApplication();
            Field critical = application.getClass().getField("UserAttentionRequestCritical");
            Field informational = application.getClass().getField("UserAttentionRequestInformational");
            Field actual = type == REQUEST_USER_ATTENTION_TYPE_CRITICAL ? critical : informational;

            return ((Integer) application.getClass().getMethod("requestUserAttention", new Class[]{Integer.TYPE}).invoke(application, new Object[]{actual.get(null)})).intValue();

        } catch (ClassNotFoundException e) {
            return -1;
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        } catch (NoSuchFieldException e) {
            throw new RuntimeException(e);
        }
    }

    public void cancelUserAttentionRequest(int request) {
        try {
            Object application = getNSApplication();
            application.getClass().getMethod("cancelUserAttentionRequest", new Class[]{Integer.TYPE}).invoke(application, new Object[]{new Integer(request)});
        } catch (ClassNotFoundException e) {
            // Nada
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    private Object getNSApplication() throws ClassNotFoundException {
        try {
            Class applicationClass = Class.forName("com.apple.cocoa.application.NSApplication");
            return applicationClass.getMethod("sharedApplication", new Class[0]).invoke(null, new Object[0]);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        }
    }

    public void setApplicationIconImage(BufferedImage image) {
        if (isMac()) {
            try {
                Method setDockIconImage = application.getClass().getMethod("setDockIconImage", Image.class);

                try {
                    setDockIconImage.invoke(application, image);
                } catch (IllegalAccessException e) {

                } catch (InvocationTargetException e) {

                }
            } catch (NoSuchMethodException mnfe) {


                ByteArrayOutputStream stream = new ByteArrayOutputStream();
                try {
                    ImageIO.write(image, "png", stream);
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }

                try {
                    Class nsDataClass = Class.forName("com.apple.cocoa.foundation.NSData");
                    Constructor constructor = nsDataClass.getConstructor(new Class[]{new byte[0].getClass()});

                    Object nsData = constructor.newInstance(new Object[]{stream.toByteArray()});

                    Class nsImageClass = Class.forName("com.apple.cocoa.application.NSImage");
                    Object nsImage = nsImageClass.getConstructor(new Class[]{nsDataClass}).newInstance(new Object[]{nsData});

                    Object application = getNSApplication();

                    application.getClass().getMethod("setApplicationIconImage", new Class[]{nsImageClass}).invoke(application, new Object[]{nsImage});

                } catch (ClassNotFoundException e) {

                } catch (NoSuchMethodException e) {
                    throw new RuntimeException(e);
                } catch (IllegalAccessException e) {
                    throw new RuntimeException(e);
                } catch (InvocationTargetException e) {
                    throw new RuntimeException(e);
                } catch (InstantiationException e) {
                    throw new RuntimeException(e);
                }

            }

        }
    }

    public BufferedImage getApplicationIconImage() {
        if (isMac()) {

            try {
                Method getDockIconImage = application.getClass().getMethod("getDockIconImage");
                try {
                    return (BufferedImage) getDockIconImage.invoke(application);
                } catch (IllegalAccessException e) {

                } catch (InvocationTargetException e) {

                }
            } catch (NoSuchMethodException nsme) {

                try {
                    Class nsDataClass = Class.forName("com.apple.cocoa.foundation.NSData");
                    Class nsImageClass = Class.forName("com.apple.cocoa.application.NSImage");
                    Object application = getNSApplication();
                    Object nsImage = application.getClass().getMethod("applicationIconImage", new Class[0]).invoke(application, new Object[0]);

                    Object nsData = nsImageClass.getMethod("TIFFRepresentation", new Class[0]).invoke(nsImage, new Object[0]);

                    Integer length = (Integer) nsDataClass.getMethod("length", new Class[0]).invoke(nsData, new Object[0]);
                    byte[] bytes = (byte[]) nsDataClass.getMethod("bytes", new Class[]{Integer.TYPE, Integer.TYPE}).invoke(nsData, new Object[]{Integer.valueOf(0), length});

                    BufferedImage image = ImageIO.read(new ByteArrayInputStream(bytes));
                    return image;

                } catch (ClassNotFoundException e) {
                    e.printStackTrace();
                } catch (NoSuchMethodException e) {
                    throw new RuntimeException(e);
                } catch (IllegalAccessException e) {
                    throw new RuntimeException(e);
                } catch (InvocationTargetException e) {
                    throw new RuntimeException(e);
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            }

        }

        return null;
    }

    private Object callMethod(String methodname) {
        return callMethod(application, methodname, new Class[0], new Object[0]);
    }

    private Object callMethod(Object object, String methodname) {
        return callMethod(object, methodname, new Class[0], new Object[0]);
    }

    private Object callMethod(Object object, String methodname, Class[] classes, Object[] arguments) {
        try {
            if (classes == null) {
                classes = new Class[arguments.length];
                for (int i = 0; i < classes.length; i++) {
                    classes[i] = arguments[i].getClass();

                }
            }
            Method addListnerMethod = object.getClass().getMethod(methodname, classes);
            return addListnerMethod.invoke(object, arguments);
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    class ApplicationListenerInvocationHandler implements InvocationHandler {
        private ApplicationListener applicationListener;

        ApplicationListenerInvocationHandler(ApplicationListener applicationListener) {
            this.applicationListener = applicationListener;
        }

        public Object invoke(Object object, Method appleMethod, Object[] objects) throws Throwable {

            ApplicationEvent event = createApplicationEvent(objects[0]);
            try {
                Method method = applicationListener.getClass().getMethod(appleMethod.getName(), new Class[]{ApplicationEvent.class});
                return method.invoke(applicationListener, new Object[]{event});
            } catch (NoSuchMethodException e) {
                if (appleMethod.getName().equals("equals") && objects.length == 1) {
                    return Boolean.valueOf(object == objects[0]);
                }
                return null;
            }
        }
    }

    private ApplicationEvent createApplicationEvent(final Object appleApplicationEvent) {
        return (ApplicationEvent) Proxy.newProxyInstance(getClass().getClassLoader(), new Class[]{ApplicationEvent.class}, new InvocationHandler() {
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                return appleApplicationEvent.getClass().getMethod(method.getName(), method.getParameterTypes()).invoke(appleApplicationEvent, objects);
            }
        });
    }
}
