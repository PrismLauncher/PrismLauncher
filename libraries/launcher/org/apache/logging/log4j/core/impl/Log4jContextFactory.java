package org.apache.logging.log4j.core.impl;

import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.spi.LoggerContextFactory;

import java.net.URI;

/**
 * This class is a stub and is needed to compile the log4j injector
 * Changing it may break compilation
 */
public class Log4jContextFactory implements LoggerContextFactory {

        @Override
        public LoggerContext getContext(String fqcn, ClassLoader loader, boolean currentContext) { 
            throw new UnsupportedOperationException();
        }
        
        @Override
        public LoggerContext getContext(String s, ClassLoader classLoader, boolean b, URI uri) {
            throw new UnsupportedOperationException();
        }

        @Override
        public LoggerContext getContext(String fqcn, ClassLoader loader, Object externalContext, boolean currentContext) {
            throw new UnsupportedOperationException();
        }
        
        @Override
        public LoggerContext getContext(String fqcn, ClassLoader loader, Object externalContext, boolean currentContext, URI configLocation, String name) {
            throw new UnsupportedOperationException();
        }
        
        @Override
        public void removeContext(org.apache.logging.log4j.spi.LoggerContext loggerContext) {}
}
