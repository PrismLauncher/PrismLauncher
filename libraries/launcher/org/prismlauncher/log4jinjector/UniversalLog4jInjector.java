package org.prismlauncher.log4jinjector;

import org.prismlauncher.utils.logging.Log;

import java.util.Objects;

public class UniversalLog4jInjector {

    private static final String EXCEPTION_DURING_INJECTION = "Unable to inject into Log4j. Token may be written to a file as a result.";

    public static void inject() {
        if (Objects.equals(System.getProperty("org.prismlauncher.log4j.inject"), "false"))
            return;
        String l4jVersion;
        try {
            l4jVersion = Class.forName("org.apache.logging.log4j.LogManager").getPackage().getImplementationVersion();
        } catch (Exception e) {
            return;
        }
        if (l4jVersion == null)
            return;
 
        if (l4jVersion.startsWith("2.0")) {
            try {
                Log4jInjector20.inject();
            } catch (Exception e) {
                Log.warning(EXCEPTION_DURING_INJECTION);
                e.printStackTrace();
            }
        } else if (l4jVersion.startsWith("2.17.1") || l4jVersion.startsWith("2.19.0")) {
            try {
                Log4jInjector2171.inject();
            } catch (Exception e) {
                Log.warning(EXCEPTION_DURING_INJECTION);
                e.printStackTrace();
            }
        } else {
            Log.warning("The active Log4j version is unsupported. Token may be written to a file as a result.");
        }
    }

}
