package org.multimc;

import org.multimc.impl.OneSixLauncher;
import org.multimc.utils.ParamBucket;

import java.util.HashMap;
import java.util.Map;

public final class LauncherFactory {

    private static final LauncherFactory INSTANCE = new LauncherFactory();

    private final Map<String, LauncherProvider> launcherRegistry = new HashMap<>();

    private LauncherFactory() {
        launcherRegistry.put("onesix", new LauncherProvider() {
            @Override
            public Launcher provide(ParamBucket parameters) {
                return new OneSixLauncher(parameters);
            }
        });
    }

    public Launcher createLauncher(String name, ParamBucket parameters) {
        LauncherProvider launcherProvider = launcherRegistry.get(name);

        if (launcherProvider == null)
            throw new IllegalArgumentException("Invalid launcher type: " + name);

        return launcherProvider.provide(parameters);
    }

    public static LauncherFactory getInstance() {
        return INSTANCE;
    }

    public interface LauncherProvider {

        Launcher provide(ParamBucket parameters);

    }

}
