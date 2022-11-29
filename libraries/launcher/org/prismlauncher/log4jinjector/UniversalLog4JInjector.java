package org.prismlauncher.log4jinjector;

import java.util.logging.Logger;
import java.util.logging.Level;
import java.util.Objects;

public class UniversalLog4JInjector {
    private static final Logger LOGGER = Logger.getLogger("UniversalLog4JInjector");

    public static void inject() {
        if (Objects.equals(System.getProperty("prism.log4j.inject"), "false")) {
            return;
        }
        String l4jVersion;
        try {
            l4jVersion = Class.forName("org.apache.logging.log4j.LogManager").getPackage().getImplementationVersion();
            assert l4jVersion != null;
        } catch (Exception e) {
            return;
        }
        if (l4jVersion.startsWith("2.0")) {
            try {
                // each log4j injector needs to be compiled against a different version of log4j
                // so they cannot be compiled together and therefore cannot be accessed directly
                Log4JInjector20.inject();
            } catch (Exception e) {
                LOGGER.log(Level.WARNING, "Unable to inject into log4j. Token may be written to a file as a result");
                e.printStackTrace();
            }
        } else if (l4jVersion.startsWith("2.17.1")) {
            try {
                Log4JInjector2171.inject();
            } catch (Exception e) {
                LOGGER.log(Level.WARNING, "Unable to inject into log4j. Token may be written to a file as a result");
                e.printStackTrace();
            }
        } else {
            LOGGER.log(Level.WARNING, "Unable to inject into log4j (Reason: log4j version not supported). Token may be written to a file as a result");
        }
    }
}
