//
// Created by robotbrain on 3/27/16.
//

#include <minecraft/MinecraftVersionList.h>
#include <Env.h>
#include <minecraft/liteloader/LiteLoaderVersionList.h>
#include <minecraft/forge/ForgeVersionList.h>
#include <minecraft/legacy/LwjglVersionList.h>
#include <java/JavaInstallList.h>
#include <settings/INISettingsObject.h>
#include <resources/Resource.h>
#include "WonkoClient.h"
#include <icons/IconList.h>


WonkoClient &WonkoClient::getInstance() {
    static WonkoClient instance;
    return instance;
}

void WonkoClient::registerLists() {
    ENV.initHttpMetaCache();
    auto mcList = std::make_shared<MinecraftVersionList>();
    ENV.registerVersionList("net.minecraft", mcList);
    runTask(mcList->getLoadTask());
    auto llList = std::make_shared<LiteLoaderVersionList>();
    ENV.registerVersionList("com.mumfrey.liteloader", llList);
    runTask(llList->getLoadTask());
    auto forgeList = std::make_shared<ForgeVersionList>();
    ENV.registerVersionList("net.minecraftforge", forgeList);
    runTask(forgeList->getLoadTask());
    auto lwjglList = std::make_shared<LWJGLVersionList>();
    ENV.registerVersionList("org.lwjgl.legacy", lwjglList);
    runTask(lwjglList->getLoadTask());
    auto javaList = std::make_shared<JavaInstallList>();
    ENV.registerVersionList("com.java", javaList);
}

WonkoClient::WonkoClient() {
    m_settings.reset(new INISettingsObject("multimc.cfg", this));
    m_instanceList.reset(new InstanceList(m_settings, ".", this));
}

void WonkoClient::runTask(Task *pTask) {
    if (pTask == nullptr)
        return;
    QEventLoop loop;
    QObject::connect(pTask, &Task::finished, &loop, &QEventLoop::quit);
    pTask->start();
    loop.exec();
    delete pTask;
}

void WonkoClient::initGlobalSettings()
{
    m_settings->registerSetting("ShowConsole", true);
    m_settings->registerSetting("RaiseConsole", true);
    m_settings->registerSetting("AutoCloseConsole", true);
    m_settings->registerSetting("LogPrePostOutput", true);
    // Window Size
    m_settings->registerSetting({"LaunchMaximized", "MCWindowMaximize"}, false);
    m_settings->registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
    m_settings->registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

    // Memory
    m_settings->registerSetting({"MinMemAlloc", "MinMemoryAlloc"}, 512);
    m_settings->registerSetting({"MaxMemAlloc", "MaxMemoryAlloc"}, 1024);
    m_settings->registerSetting("PermGen", 128);

    // Java Settings
    m_settings->registerSetting("JavaPath", "");
    m_settings->registerSetting("JavaTimestamp", 0);
    m_settings->registerSetting("JavaVersion", "");
    m_settings->registerSetting("LastHostname", "");
    m_settings->registerSetting("JavaDetectionHack", "");
    m_settings->registerSetting("JvmArgs", "");

    // Wrapper command for launch
    m_settings->registerSetting("WrapperCommand", "");

    // Custom Commands
    m_settings->registerSetting({"PreLaunchCommand", "PreLaunchCmd"}, "");
    m_settings->registerSetting({"PostExitCommand", "PostExitCmd"}, "");
}
