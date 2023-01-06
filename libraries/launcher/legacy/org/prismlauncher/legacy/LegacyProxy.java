package org.prismlauncher.legacy;

import org.prismlauncher.launcher.Launcher;
import org.prismlauncher.legacy.fix.online.OnlineFixes;
import org.prismlauncher.utils.Parameters;

public final class LegacyProxy {

    public static Launcher createLauncher(Parameters params) {
        return new LegacyLauncher(params);
    }

    public static void applyOnlineFixes(Parameters parameters) {
        OnlineFixes.apply(parameters);
    }

}
