package org.prismlauncher.log4jinjector;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.core.Logger;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.impl.Log4jContextFactory;
import org.apache.logging.log4j.message.MessageFactory;

import java.lang.reflect.Field;
import java.net.URI;

public class Log4jInjector20 {

    public static void inject() {
        try {
            Field f = LogManager.class.getDeclaredField("factory");
            f.setAccessible(true);
            f.set(null, new WrappedLoggerContextFactory((Log4jContextFactory) LogManager.getFactory()));
        } catch (NoSuchFieldException | IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }

    static class WrappedLoggerContextFactory extends Log4jContextFactory {
        private final Log4jContextFactory delegate;

        public WrappedLoggerContextFactory(Log4jContextFactory delegate) {
            this.delegate = delegate;
        }

        @Override
        public LoggerContext getContext(String fqcn, ClassLoader loader, boolean currentContext) {
            return new WrappedLoggerContext(delegate.getContext(fqcn, loader, currentContext));
        }

        @Override
        public LoggerContext getContext(String s, ClassLoader classLoader, boolean b, URI uri) {
            return new WrappedLoggerContext(delegate.getContext(s, classLoader, b, uri));
        }

        @Override
        public void removeContext(org.apache.logging.log4j.spi.LoggerContext loggerContext) {
            delegate.removeContext(loggerContext);
        }
    }

    static class WrappedLoggerContext extends LoggerContext {
        private final LoggerContext delegate;

        public WrappedLoggerContext(LoggerContext delegate) {
            super(delegate.getName(), delegate.getExternalContext());
            this.delegate = delegate;
        }

        @Override
        public Object getExternalContext() {
            return delegate.getExternalContext();
        }

        @Override
        public Logger getLogger(final String name) {
            return new WrappedLogger(delegate.getLogger(name));
        }

        @Override
        public Logger getLogger(String name, MessageFactory messageFactory) {
            return new WrappedLogger(delegate.getLogger(name, messageFactory));
        }

        @Override
        public boolean hasLogger(String name) {
            return delegate.hasLogger(name);
        }

    }

    public static class WrappedLogger extends Logger {

        private final Logger delegate;

        public WrappedLogger(Logger delegate) {
            super(delegate.getContext(), delegate.getName(), delegate.getMessageFactory());
            this.delegate = delegate;
        }

        @Override
        public void info(String message) {
            if (message.startsWith("(Session ID is ")) {
                return;
            }
            delegate.info(message);
        }
        
        @Override
        public void debug(String message) {
            if (message.startsWith("(Session ID is ")) {
                return;
            }
            delegate.info(message);
        }

    }

}
