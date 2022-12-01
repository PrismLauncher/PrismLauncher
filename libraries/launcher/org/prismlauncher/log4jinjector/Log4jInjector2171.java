package org.prismlauncher.log4jinjector;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.core.Logger;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.impl.Log4jContextFactory;
import org.apache.logging.log4j.message.MessageFactory;

import java.net.URI;

public class Log4jInjector2171 {

    public static void inject() {
        LogManager.setFactory(
            new WrappedLoggerContextFactory((Log4jContextFactory) LogManager.getFactory())
        );
    }

    static class WrappedLoggerContextFactory extends Log4jContextFactory {
        private final Log4jContextFactory delegate;

        public WrappedLoggerContextFactory(Log4jContextFactory delegate) {
            this.delegate = delegate;
        }

        @Override
        public LoggerContext getContext(String fqcn, ClassLoader loader, Object externalContext, boolean currentContext) {
            return new WrappedLoggerContext(delegate.getContext(fqcn, loader, externalContext, currentContext));
        }

        @Override
        public LoggerContext getContext(String fqcn, ClassLoader loader, Object externalContext, boolean currentContext, URI configLocation, String name) {
            return new WrappedLoggerContext(delegate.getContext(fqcn, loader, externalContext, currentContext));
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
        public Logger getLogger(String name) {
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

        @Override
        public boolean hasLogger(String name, MessageFactory messageFactory) {
            return delegate.hasLogger(name, messageFactory);
        }

        @Override
        public boolean hasLogger(String name, Class<? extends MessageFactory> messageFactoryClass) {
            return delegate.hasLogger(name, messageFactoryClass);
        }
    }

    static class WrappedLogger extends Logger {
        private final Logger delegate;

        protected WrappedLogger(Logger delegate) {
            super(delegate.getContext(), delegate.getName(), delegate.getMessageFactory());
            this.delegate = delegate;
        }

        @Override
        public void info(String message) {
            if (message.startsWith("(Session ID is "))
                return;

            delegate.info(message);
        }

    }
}
