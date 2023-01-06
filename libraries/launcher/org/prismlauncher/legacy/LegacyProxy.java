package org.prismlauncher.legacy;

import org.prismlauncher.launcher.Launcher;
import org.prismlauncher.utils.Parameters;

// used as a fallback if NewLaunchLegacy is not on the classpath
// if it is, this class will be replaced
public final class LegacyProxy {

    public static Launcher createLauncher(Parameters params) {
        throw new AssertionError("NewLaunchLegacy is not loaded");
    }

    public static void applyOnlineFixes(Parameters params) {
    }

}
