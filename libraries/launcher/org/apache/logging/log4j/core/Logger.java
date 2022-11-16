package org.apache.logging.log4j.core;

import org.apache.logging.log4j.message.MessageFactory;

/**
 * This class is a stub and is needed to compile the log4j injector
 * Changing it may break things
 **/
public class Logger {
    public Logger(LoggerContext a, String b, MessageFactory m) {}
    public void info(String message) {}
    public LoggerContext getContext() {
        throw new RuntimeException("This is a stub");
    }
    public String getName() {
        throw new RuntimeException("This is a stub");
    }
    public MessageFactory getMessageFactory() {
        throw new RuntimeException("This is a stub");
    }
}