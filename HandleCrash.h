// This is a simple header file for the crash handling system. It exposes only one method,
// initBlackMagic, which initializes the system, registering signal handlers, or doing
// whatever stupid things need to be done on Windows.
// The platform specific implementations for this system are in UnixCrash.cpp and 
// WinCrash.cpp.

#if defined Q_OS_WIN
#warning Crash handling is not yet implemented on Windows.
#elif defined Q_OS_UNIX
#else
#warning Crash handling is not supported on this platform.
#endif

/**
 * Initializes the crash handling system.
 */
void initBlackMagic();

