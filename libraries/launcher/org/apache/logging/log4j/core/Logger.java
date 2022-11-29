package org.apache.logging.log4j.core;

import org.apache.logging.log4j.message.MessageFactory;

/**
 * This class is a stub and is needed to compile the log4j injector
 * Changing it may break things
 */
public class Logger {
    public Logger(LoggerContext a, String b, MessageFactory m) {}
    public void info(String message) {}
    public void debug(String message) {}
    public LoggerContext getContext() {
        throw new UnsupportedOperationException();
    }
    public String getName() {
        throw new UnsupportedOperationException();
    }
    public MessageFactory getMessageFactory() {
        throw new UnsupportedOperationException();
    }
}