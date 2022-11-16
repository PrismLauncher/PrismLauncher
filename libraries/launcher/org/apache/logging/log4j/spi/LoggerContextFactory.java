package org.apache.logging.log4j.spi;

import org.apache.logging.log4j.core.LoggerContext;
import java.net.URI;

/**
 * This class is a stub and is needed to compile the log4j injector
 * Changing it may break things
 **/
public interface LoggerContextFactory {
        public LoggerContext getContext(String fqcn, ClassLoader loader, boolean currentContext);

        public LoggerContext getContext(String s, ClassLoader classLoader, boolean b, URI uri);
        public LoggerContext getContext(String fqcn, ClassLoader loader, Object externalContext, boolean currentContext);
        public LoggerContext getContext(String fqcn, ClassLoader loader, Object externalContext, boolean currentContext, URI configLocation, String name);

        public void removeContext(org.apache.logging.log4j.spi.LoggerContext loggerContext);
}