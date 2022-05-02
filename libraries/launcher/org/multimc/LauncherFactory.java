package org.multimc;

import org.multimc.impl.OneSixLauncher;
import org.multimc.utils.ParamBucket;

import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;

public final class LauncherFactory {

    private static final LauncherFactory INSTANCE = new LauncherFactory();

    private final Map<String, Function<ParamBucket, Launcher>> launcherRegistry = new HashMap<>();

    private LauncherFactory() {
        launcherRegistry.put("onesix", OneSixLauncher::new);
    }

    public Launcher createLauncher(String name, ParamBucket parameters) {
        Function<ParamBucket, Launcher> launcherCreator =
                launcherRegistry.get(name);

        if (launcherCreator == null)
            throw new IllegalArgumentException("Invalid launcher type: " + name);

        return launcherCreator.apply(parameters);
    }

    public static LauncherFactory getInstance() {
        return INSTANCE;
    }

}
