package org.apache.logging.log4j;

import org.apache.logging.log4j.spi.LoggerContextFactory;
/**
 * This class is a stub and is needed to compile the log4j injector
 * Changing it may break things
 **/
public class LogManager {
    public static void setFactory(LoggerContextFactory l) {}

    public static LoggerContextFactory getFactory() {
        throw new RuntimeException("This is a stub");
    }
}