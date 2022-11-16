package org.apache.logging.log4j.core;

import org.apache.logging.log4j.message.MessageFactory;

/**
 * This class is a stub and is needed to compile the log4j injector
 * Changing it may break things
 **/
public class LoggerContext {
    public LoggerContext(String a, Object b) {}

    public Object getExternalContext() {
        throw new RuntimeException("This is a stub");
    }

    public String getName() {
        throw new RuntimeException("This is a stub");    
    }
    
    public Logger getLogger(final String name) {
        throw new RuntimeException("This is a stub");
    }

    public Logger getLogger(String name, MessageFactory messageFactory) {
        throw new RuntimeException("This is a stub");
    }

    public boolean hasLogger(String name) {
        throw new RuntimeException("This is a stub");
    }

    public boolean hasLogger(String name, MessageFactory messageFactory) {
        throw new RuntimeException("This is a stub");
    }

    public boolean hasLogger(String name, Class<? extends MessageFactory> messageFactoryClass) {
        throw new RuntimeException("This is a stub");
    }
}