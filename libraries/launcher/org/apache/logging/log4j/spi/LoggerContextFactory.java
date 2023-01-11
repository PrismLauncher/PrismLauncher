package org.apache.logging.log4j.spi;

import org.apache.logging.log4j.core.LoggerContext;
import java.net.URI;

/**
 * This class is a stub and is needed to compile the log4j injector
 * Changing it may break things
 */
public interface LoggerContextFactory {
        LoggerContext getContext(String fqcn, ClassLoader loader, boolean currentContext);

        LoggerContext getContext(String s, ClassLoader classLoader, boolean b, URI uri);
        LoggerContext getContext(String fqcn, ClassLoader loader, Object externalContext, boolean currentContext);

        LoggerContext getContext(String fqcn, ClassLoader loader, Object externalContext, boolean currentContext, URI configLocation, String name);

        void removeContext(org.apache.logging.log4j.spi.LoggerContext loggerContext);
}
