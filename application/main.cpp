#include "MultiMC.h"
#include "MainWindow.h"
#include "LaunchController.h"
#include <InstanceList.h>
#include <QDebug>

#ifdef Q_OS_WIN
    #include<windows.h>
#endif

// #define BREAK_INFINITE_LOOP
// #define BREAK_EXCEPTION
// #define BREAK_RETURN

#ifdef BREAK_INFINITE_LOOP
#include <thread>
#include <chrono>
#endif

int main(int argc, char *argv[])
{
#ifdef BREAK_INFINITE_LOOP
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
#endif
#ifdef BREAK_EXCEPTION
    throw 42;
#endif
#ifdef BREAK_RETURN
    return 42;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    #ifdef Q_OS_WIN
        BOOL (__stdcall *pFn)(void);
        HINSTANCE hInstance=LoadLibrary("user32.dll");
        if(hInstance) {
            pFn = (BOOL (__stdcall*)(void))GetProcAddress(hInstance, "SetProcessDPIAware");
            if(pFn)
                pFn();
            FreeLibrary(hInstance);
        }
        QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    #else
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // initialize Qt
    MultiMC app(argc, argv);

    switch (app.status())
    {
    case MultiMC::StartingUp:
    case MultiMC::Initialized:
    {
        Q_INIT_RESOURCE(multimc);
        Q_INIT_RESOURCE(backgrounds);

        Q_INIT_RESOURCE(pe_dark);
        Q_INIT_RESOURCE(pe_light);
        Q_INIT_RESOURCE(pe_blue);
        Q_INIT_RESOURCE(pe_colored);
        Q_INIT_RESOURCE(OSX);
        Q_INIT_RESOURCE(iOS);
        Q_INIT_RESOURCE(flat);
        return app.exec();
    }
    case MultiMC::Failed:
        return 1;
    case MultiMC::Succeeded:
        return 0;
    }
}
