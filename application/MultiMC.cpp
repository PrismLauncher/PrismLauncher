#include "MultiMC.h"
#include "BuildConfig.h"
#include "MainWindow.h"
#include "InstanceWindow.h"
#include "pages/BasePageProvider.h"
#include "pages/global/MultiMCSettingsPage.h"
#include "pages/global/MinecraftPage.h"
#include "pages/global/JavaPage.h"
#include "pages/global/LanguagePage.h"
#include "pages/global/ProxyPage.h"
#include "pages/global/ExternalToolsPage.h"
#include "pages/global/AccountListPage.h"
#include "pages/global/PasteEEPage.h"
#include "pages/global/PackagesPage.h"
#include "pages/global/CustomCommandsPage.h"

#include "themes/ITheme.h"
#include "themes/SystemTheme.h"
#include "themes/DarkTheme.h"
#include "themes/BrightTheme.h"
#include "themes/CustomTheme.h"

#include "setupwizard/SetupWizard.h"
#include "setupwizard/LanguageWizardPage.h"
#include "setupwizard/JavaWizardPage.h"
#include "setupwizard/AnalyticsWizardPage.h"

#include <iostream>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTranslator>
#include <QLibraryInfo>
#include <QStringList>
#include <QDebug>
#include <QStyleFactory>

#include "dialogs/CustomMessageBox.h"
#include "InstanceList.h"

#include <minecraft/auth/MojangAccountList.h>
#include "icons/IconList.h"
#include "net/HttpMetaCache.h"
#include "net/URLConstants.h"
#include "Env.h"

#include "java/JavaUtils.h"

#include "updater/UpdateChecker.h"

#include "tools/JProfiler.h"
#include "tools/JVisualVM.h"
#include "tools/MCEditTool.h"

#include <xdgicon.h>
#include "settings/INISettingsObject.h"
#include "settings/Setting.h"

#include "translations/TranslationsModel.h"

#include <Commandline.h>
#include <FileSystem.h>
#include <DesktopServices.h>
#include <LocalPeer.h>

#include <ganalytics.h>
#include <sys.h>

#include "pagedialog/PageDialog.h"


#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static const QLatin1String liveCheckFile("live.check");

using namespace Commandline;

#define MACOS_HINT "If you are on macOS Sierra, you might have to move MultiMC.app to your /Applications or ~/Applications folder. "\
    "This usually fixes the problem and you can move the application elsewhere afterwards.\n"\
    "\n"

static void appDebugOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const char *levels = "DWCFIS";
    const QString format("%1 %2 %3\n");

    qint64 msecstotal = MMC->timeSinceStart();
    qint64 seconds = msecstotal / 1000;
    qint64 msecs = msecstotal % 1000;
    QString foo;
    char buf[1025] = {0};
    ::snprintf(buf, 1024, "%5lld.%03lld", seconds, msecs);

    QString out = format.arg(buf).arg(levels[type]).arg(msg);

    MMC->logFile->write(out.toUtf8());
    MMC->logFile->flush();
    QTextStream(stderr) << out.toLocal8Bit();
    fflush(stderr);
}

MultiMC::MultiMC(int &argc, char **argv) : QApplication(argc, argv)
{
#if defined Q_OS_WIN32
    // attach the parent console
    if(AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // if attach succeeds, reopen and sync all the i/o
        if(freopen("CON", "w", stdout))
        {
            std::cout.sync_with_stdio();
        }
        if(freopen("CON", "w", stderr))
        {
            std::cerr.sync_with_stdio();
        }
        if(freopen("CON", "r", stdin))
        {
            std::cin.sync_with_stdio();
        }
        auto out = GetStdHandle (STD_OUTPUT_HANDLE);
        DWORD written;
        const char * endline = "\n";
        WriteConsole(out, endline, strlen(endline), &written, NULL);
        consoleAttached = true;
    }
#endif
    setOrganizationName("MultiMC");
    setOrganizationDomain("multimc.org");
    setApplicationName("MultiMC5");
    setApplicationDisplayName("MultiMC 5");
    setApplicationVersion(BuildConfig.printableVersionString());

    startTime = QDateTime::currentDateTime();

    // Don't quit on hiding the last window
    this->setQuitOnLastWindowClosed(false);

    // Commandline parsing
    QHash<QString, QVariant> args;
    {
        Parser parser(FlagStyle::GNU, ArgumentStyle::SpaceAndEquals);

        // --help
        parser.addSwitch("help");
        parser.addShortOpt("help", 'h');
        parser.addDocumentation("help", "display this help and exit.");
        // --version
        parser.addSwitch("version");
        parser.addShortOpt("version", 'V');
        parser.addDocumentation("version", "display program version and exit.");
        // --dir
        parser.addOption("dir");
        parser.addShortOpt("dir", 'd');
        parser.addDocumentation("dir", "use the supplied folder as MultiMC root instead of "
                                       "the binary location (use '.' for current)");
        // --launch
        parser.addOption("launch");
        parser.addShortOpt("launch", 'l');
        parser.addDocumentation("launch", "launch the specified instance (by instance ID)");
        // --alive
        parser.addSwitch("alive");
        parser.addDocumentation("alive", "write a small '" + liveCheckFile + "' file after MultiMC starts");

        // parse the arguments
        try
        {
            args = parser.parse(arguments());
        }
        catch (const ParsingError &e)
        {
            std::cerr << "CommandLineError: " << e.what() << std::endl;
            std::cerr << "Try '%1 -h' to get help on MultiMC's command line parameters."
                      << std::endl;
            m_status = MultiMC::Failed;
            return;
        }

        // display help and exit
        if (args["help"].toBool())
        {
            std::cout << qPrintable(parser.compileHelp(arguments()[0]));
            m_status = MultiMC::Succeeded;
            return;
        }

        // display version and exit
        if (args["version"].toBool())
        {
            std::cout << "Version " << BuildConfig.printableVersionString().toStdString() << std::endl;
            std::cout << "Git " << BuildConfig.GIT_COMMIT.toStdString() << std::endl;
            m_status = MultiMC::Succeeded;
            return;
        }
    }
    m_instanceIdToLaunch = args["launch"].toString();
    m_liveCheck = args["alive"].toBool();

    QString origcwdPath = QDir::currentPath();
    QString binPath = applicationDirPath();
    QString adjustedBy;
    QString dataPath;
    // change folder
    QString dirParam = args["dir"].toString();
    if (!dirParam.isEmpty())
    {
        // the dir param. it makes multimc data path point to whatever the user specified
        // on command line
        adjustedBy += "Command line " + dirParam;
        dataPath = dirParam;
    }
    else
    {
#ifdef MULTIMC_LINUX_DATADIR
        QString xdgDataHome = QFile::decodeName(qgetenv("XDG_DATA_HOME"));
        if (xdgDataHome.isEmpty())
            xdgDataHome = QDir::homePath() + QLatin1String("/.local/share");
        dataPath = xdgDataHome + "/multimc";
        adjustedBy += "XDG standard " + dataPath;
#else
        dataPath = applicationDirPath();
        adjustedBy += "Fallback to binary path " + dataPath;
#endif
    }

    if (!FS::ensureFolderPathExists(dataPath))
    {
        showFatalErrorMessage(
            "MultiMC data folder could not be created.",
            "MultiMC data folder could not be created.\n"
            "\n"
#if defined(Q_OS_MAC)
            MACOS_HINT
#endif
            "Make sure you have the right permissions to the MultiMC data folder and any folder needed to access it.\n"
            "\n"
            "MultiMC cannot continue until you fix this problem."
        );
        return;
    }
    if (!QDir::setCurrent(dataPath))
    {
        showFatalErrorMessage(
            "MultiMC data folder could not be opened.",
            "MultiMC data folder could not be opened.\n"
            "\n"
#if defined(Q_OS_MAC)
            MACOS_HINT
#endif
            "Make sure you have the right permissions to the MultiMC data folder.\n"
            "\n"
            "MultiMC cannot continue until you fix this problem."
        );
        return;
    }

    /*
     * Establish the mechanism for communication with an already running MultiMC that uses the same data path.
     * If there is one, tell it what the user actually wanted to do and exit.
     * We want to initialize this before logging to avoid messing with the log of a potential already running copy.
     */
    auto appID = ApplicationId::fromPathAndVersion(QDir::currentPath(), BuildConfig.printableVersionString());
    {
        // FIXME: you can run the same binaries with multiple data dirs and they won't clash. This could cause issues for updates.
        m_peerInstance = new LocalPeer(this, appID);
        connect(m_peerInstance, &LocalPeer::messageReceived, this, &MultiMC::messageReceived);
        if(m_peerInstance->isClient())
        {
            if(m_instanceIdToLaunch.isEmpty())
            {
                m_peerInstance->sendMessage("activate", 2000);
            }
            else
            {
                m_peerInstance->sendMessage(m_instanceIdToLaunch, 2000);
            }
            m_status = MultiMC::Succeeded;
            return;
        }
    }

    // init the logger
    {
        static const QString logBase = "MultiMC-%0.log";
        auto moveFile = [](const QString &oldName, const QString &newName)
        {
            QFile::remove(newName);
            QFile::copy(oldName, newName);
            QFile::remove(oldName);
        };

        moveFile(logBase.arg(3), logBase.arg(4));
        moveFile(logBase.arg(2), logBase.arg(3));
        moveFile(logBase.arg(1), logBase.arg(2));
        moveFile(logBase.arg(0), logBase.arg(1));

        logFile = std::unique_ptr<QFile>(new QFile(logBase.arg(0)));
        if(!logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            showFatalErrorMessage(
                "MultiMC data folder is not writable!",
                "MultiMC couldn't create a log file - the MultiMC data folder is not writable.\n"
                "\n"
    #if defined(Q_OS_MAC)
                MACOS_HINT
    #endif
                "Make sure you have write permissions to the MultiMC data folder.\n"
                "\n"
                "MultiMC cannot continue until you fix this problem."
            );
            return;
        }
        qInstallMessageHandler(appDebugOutput);
        qDebug() << "<> Log initialized.";
    }

    // Set up paths
    {
        // Root path is used for updates.
#ifdef Q_OS_LINUX
        QDir foo(FS::PathCombine(binPath, ".."));
        m_rootPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
        m_rootPath = binPath;
#elif defined(Q_OS_MAC)
        QDir foo(FS::PathCombine(binPath, "../.."));
        m_rootPath = foo.absolutePath();
        // on macOS, touch the root to force Finder to reload the .app metadata (and fix any icon change issues)
        FS::updateTimestamp(m_rootPath);
#endif

#ifdef MULTIMC_JARS_LOCATION
        ENV.setJarsPath( TOSTRING(MULTIMC_JARS_LOCATION) );
#endif

        qDebug() << "MultiMC 5, (c) 2013-2019 MultiMC Contributors";
        qDebug() << "Version                    : " << BuildConfig.printableVersionString();
        qDebug() << "Git commit                 : " << BuildConfig.GIT_COMMIT;
        qDebug() << "Git refspec                : " << BuildConfig.GIT_REFSPEC;
        if (adjustedBy.size())
        {
            qDebug() << "Work dir before adjustment : " << origcwdPath;
            qDebug() << "Work dir after adjustment  : " << QDir::currentPath();
            qDebug() << "Adjusted by                : " << adjustedBy;
        }
        else
        {
            qDebug() << "Work dir                   : " << QDir::currentPath();
        }
        qDebug() << "Binary path                : " << binPath;
        qDebug() << "Application root path      : " << m_rootPath;
        if(!m_instanceIdToLaunch.isEmpty())
        {
            qDebug() << "ID of instance to launch   : " << m_instanceIdToLaunch;
        }
        qDebug() << "<> Paths set.";
    }

    do // once
    {
        if(m_liveCheck)
        {
            QFile check(liveCheckFile);
            if(!check.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                qWarning() << "Could not open" << liveCheckFile << "for writing!";
                break;
            }
            auto payload = appID.toString().toUtf8();
            if(check.write(payload) != payload.size())
            {
                qWarning() << "Could not write into" << liveCheckFile;
                check.remove();
                break;
            }
            check.close();
        }
    } while(false);

    // Initialize application settings
    {
        m_settings.reset(new INISettingsObject("multimc.cfg", this));
        // Updates
        m_settings->registerSetting("UpdateChannel", BuildConfig.VERSION_CHANNEL);
        m_settings->registerSetting("AutoUpdate", true);

        // Theming
        m_settings->registerSetting("IconTheme", QString("multimc"));
        m_settings->registerSetting("ApplicationTheme", QString("system"));

        // Notifications
        m_settings->registerSetting("ShownNotifications", QString());

        // Remembered state
        m_settings->registerSetting("LastUsedGroupForNewInstance", QString());

        QString defaultMonospace;
        int defaultSize = 11;
#ifdef Q_OS_WIN32
        defaultMonospace = "Courier";
        defaultSize = 10;
#elif defined(Q_OS_MAC)
        defaultMonospace = "Menlo";
#else
        defaultMonospace = "Monospace";
#endif

        // resolve the font so the default actually matches
        QFont consoleFont;
        consoleFont.setFamily(defaultMonospace);
        consoleFont.setStyleHint(QFont::Monospace);
        consoleFont.setFixedPitch(true);
        QFontInfo consoleFontInfo(consoleFont);
        QString resolvedDefaultMonospace = consoleFontInfo.family();
        QFont resolvedFont(resolvedDefaultMonospace);
        qDebug() << "Detected default console font:" << resolvedDefaultMonospace
            << ", substitutions:" << resolvedFont.substitutions().join(',');

        m_settings->registerSetting("ConsoleFont", resolvedDefaultMonospace);
        m_settings->registerSetting("ConsoleFontSize", defaultSize);
        m_settings->registerSetting("ConsoleMaxLines", 100000);
        m_settings->registerSetting("ConsoleOverflowStop", true);

        // Folders
        m_settings->registerSetting("InstanceDir", "instances");
        m_settings->registerSetting({"CentralModsDir", "ModsDir"}, "mods");
        m_settings->registerSetting("IconsDir", "icons");

        // Editors
        m_settings->registerSetting("JsonEditor", QString());

        // Language
        m_settings->registerSetting("Language", QString());

        // Console
        m_settings->registerSetting("ShowConsole", false);
        m_settings->registerSetting("AutoCloseConsole", false);
        m_settings->registerSetting("ShowConsoleOnError", true);
        m_settings->registerSetting("LogPrePostOutput", true);

        // Window Size
        m_settings->registerSetting({"LaunchMaximized", "MCWindowMaximize"}, false);
        m_settings->registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
        m_settings->registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

        // Proxy Settings
        m_settings->registerSetting("ProxyType", "None");
        m_settings->registerSetting({"ProxyAddr", "ProxyHostName"}, "127.0.0.1");
        m_settings->registerSetting("ProxyPort", 8080);
        m_settings->registerSetting({"ProxyUser", "ProxyUsername"}, "");
        m_settings->registerSetting({"ProxyPass", "ProxyPassword"}, "");

        // Memory
        m_settings->registerSetting({"MinMemAlloc", "MinMemoryAlloc"}, 512);
        m_settings->registerSetting({"MaxMemAlloc", "MaxMemoryAlloc"}, 1024);
        m_settings->registerSetting("PermGen", 128);

        // Java Settings
        m_settings->registerSetting("JavaPath", "");
        m_settings->registerSetting("JavaTimestamp", 0);
        m_settings->registerSetting("JavaArchitecture", "");
        m_settings->registerSetting("JavaVersion", "");
        m_settings->registerSetting("LastHostname", "");
        m_settings->registerSetting("JvmArgs", "");

        // Minecraft launch method
        m_settings->registerSetting("MCLaunchMethod", "LauncherPart");

        // Wrapper command for launch
        m_settings->registerSetting("WrapperCommand", "");

        // Custom Commands
        m_settings->registerSetting({"PreLaunchCommand", "PreLaunchCmd"}, "");
        m_settings->registerSetting({"PostExitCommand", "PostExitCmd"}, "");

        // The cat
        m_settings->registerSetting("TheCat", false);

        m_settings->registerSetting("InstSortMode", "Name");
        m_settings->registerSetting("SelectedInstance", QString());

        // Window state and geometry
        m_settings->registerSetting("MainWindowState", "");
        m_settings->registerSetting("MainWindowGeometry", "");

        m_settings->registerSetting("ConsoleWindowState", "");
        m_settings->registerSetting("ConsoleWindowGeometry", "");

        m_settings->registerSetting("SettingsGeometry", "");

        m_settings->registerSetting("PagedGeometry", "");

        m_settings->registerSetting("NewInstanceGeometry", "");

        m_settings->registerSetting("UpdateDialogGeometry", "");

        // paste.ee API key
        m_settings->registerSetting("PasteEEAPIKey", "multimc");

        if(!BuildConfig.ANALYTICS_ID.isEmpty())
        {
            // Analytics
            m_settings->registerSetting("Analytics", true);
            m_settings->registerSetting("AnalyticsSeen", 0);
            m_settings->registerSetting("AnalyticsClientID", QString());
        }

        // Init page provider
        {
            m_globalSettingsProvider = std::make_shared<GenericPageProvider>(tr("Settings"));
            m_globalSettingsProvider->addPage<MultiMCSettingsPage>();
            m_globalSettingsProvider->addPage<MinecraftPage>();
            m_globalSettingsProvider->addPage<JavaPage>();
            m_globalSettingsProvider->addPage<LanguagePage>();
            m_globalSettingsProvider->addPage<CustomCommandsPage>();
            m_globalSettingsProvider->addPage<ProxyPage>();
            // m_globalSettingsProvider->addPage<PackagesPage>();
            m_globalSettingsProvider->addPage<ExternalToolsPage>();
            m_globalSettingsProvider->addPage<AccountListPage>();
            m_globalSettingsProvider->addPage<PasteEEPage>();
        }
        qDebug() << "<> Settings loaded.";
    }

    // load translations
    {
        m_translations.reset(new TranslationsModel("translations"));
        auto bcp47Name = m_settings->get("Language").toString();
        m_translations->selectLanguage(bcp47Name);
        qDebug() << "Your language is" << bcp47Name;
        qDebug() << "<> Translations loaded.";
    }

    // initialize the updater
    if(BuildConfig.UPDATER_ENABLED)
    {
        m_updateChecker.reset(new UpdateChecker(BuildConfig.CHANLIST_URL, BuildConfig.VERSION_CHANNEL, BuildConfig.VERSION_BUILD));
        qDebug() << "<> Updater started.";
    }

    // Instance icons
    {
        auto setting = MMC->settings()->getSetting("IconsDir");
        QStringList instFolders =
        {
            ":/icons/multimc/32x32/instances/",
            ":/icons/multimc/50x50/instances/",
            ":/icons/multimc/128x128/instances/",
            ":/icons/multimc/scalable/instances/"
        };
        m_icons.reset(new IconList(instFolders, setting->get().toString()));
        connect(setting.get(), &Setting::SettingChanged,[&](const Setting &, QVariant value)
        {
            m_icons->directoryChanged(value.toString());
        });
        ENV.registerIconList(m_icons);
        qDebug() << "<> Instance icons intialized.";
    }

    // Icon themes
    {
        // TODO: icon themes and instance icons do not mesh well together. Rearrange and fix discrepancies!
        // set icon theme search path!
        auto searchPaths = QIcon::themeSearchPaths();
        searchPaths.append("iconthemes");
        QIcon::setThemeSearchPaths(searchPaths);
        qDebug() << "<> Icon themes initialized.";
    }

    // Initialize widget themes
    {
        auto insertTheme = [this](ITheme * theme)
        {
            m_themes.insert(std::make_pair(theme->id(), std::unique_ptr<ITheme>(theme)));
        };
        auto darkTheme = new DarkTheme();
        insertTheme(new SystemTheme());
        insertTheme(darkTheme);
        insertTheme(new BrightTheme());
        insertTheme(new CustomTheme(darkTheme, "custom"));
        qDebug() << "<> Widget themes initialized.";
    }

    // initialize and load all instances
    {
        auto InstDirSetting = m_settings->getSetting("InstanceDir");
        // instance path: check for problems with '!' in instance path and warn the user in the log
        // and rememer that we have to show him a dialog when the gui starts (if it does so)
        QString instDir = InstDirSetting->get().toString();
        qDebug() << "Instance path              : " << instDir;
        if (FS::checkProblemticPathJava(QDir(instDir)))
        {
            qWarning() << "Your instance path contains \'!\' and this is known to cause java problems";
        }
        m_instances.reset(new InstanceList(m_settings, instDir, this));
        connect(InstDirSetting.get(), &Setting::SettingChanged, m_instances.get(), &InstanceList::on_InstFolderChanged);
        qDebug() << "Loading Instances...";
        m_instances->loadList();
        qDebug() << "<> Instances loaded.";
    }

    // and accounts
    {
        m_accounts.reset(new MojangAccountList(this));
        qDebug() << "Loading accounts...";
        m_accounts->setListFilePath("accounts.json", true);
        m_accounts->loadList();
        qDebug() << "<> Accounts loaded.";
    }

    // init the http meta cache
    {
        ENV.initHttpMetaCache();
        qDebug() << "<> Cache initialized.";
    }

    // init proxy settings
    {
        QString proxyTypeStr = settings()->get("ProxyType").toString();
        QString addr = settings()->get("ProxyAddr").toString();
        int port = settings()->get("ProxyPort").value<qint16>();
        QString user = settings()->get("ProxyUser").toString();
        QString pass = settings()->get("ProxyPass").toString();
        ENV.updateProxySettings(proxyTypeStr, addr, port, user, pass);
        qDebug() << "<> Proxy settings done.";
    }

    // now we have network, download translation updates
    m_translations->downloadIndex();

    //FIXME: what to do with these?
    m_profilers.insert("jprofiler", std::shared_ptr<BaseProfilerFactory>(new JProfilerFactory()));
    m_profilers.insert("jvisualvm", std::shared_ptr<BaseProfilerFactory>(new JVisualVMFactory()));
    for (auto profiler : m_profilers.values())
    {
        profiler->registerSettings(m_settings);
    }

    // Create the MCEdit thing... why is this here?
    {
        m_mcedit.reset(new MCEditTool(m_settings));
    }

    connect(this, &MultiMC::aboutToQuit, [this](){
        if(m_instances)
        {
            // save any remaining instance state
            m_instances->saveNow();
        }
        if(logFile)
        {
            logFile->flush();
            logFile->close();
        }
    });

    {
        setIconTheme(settings()->get("IconTheme").toString());
        qDebug() << "<> Icon theme set.";
        setApplicationTheme(settings()->get("ApplicationTheme").toString(), true);
        qDebug() << "<> Application theme set.";
    }

    // Initialize analytics
    [this]()
    {
        const int analyticsVersion = 2;
        if(BuildConfig.ANALYTICS_ID.isEmpty())
        {
            return;
        }

        auto analyticsSetting = m_settings->getSetting("Analytics");
        connect(analyticsSetting.get(), &Setting::SettingChanged, this, &MultiMC::analyticsSettingChanged);
        QString clientID = m_settings->get("AnalyticsClientID").toString();
        if(clientID.isEmpty())
        {
            clientID = QUuid::createUuid().toString();
            clientID.remove(QLatin1Char('{'));
            clientID.remove(QLatin1Char('}'));
            m_settings->set("AnalyticsClientID", clientID);
        }
        m_analytics = new GAnalytics(BuildConfig.ANALYTICS_ID, clientID, analyticsVersion, this);
        m_analytics->setLogLevel(GAnalytics::Debug);
        m_analytics->setAnonymizeIPs(true);
        m_analytics->setNetworkAccessManager(&ENV.qnam());

        if(m_settings->get("AnalyticsSeen").toInt() < m_analytics->version())
        {
            qDebug() << "Analytics info not seen by user yet (or old version).";
            return;
        }
        if(!m_settings->get("Analytics").toBool())
        {
            qDebug() << "Analytics disabled by user.";
            return;
        }

        m_analytics->enable();
        qDebug() << "<> Initialized analytics with tid" << BuildConfig.ANALYTICS_ID;
    }();

    if(createSetupWizard())
    {
        return;
    }
    performMainStartupAction();
}

bool MultiMC::createSetupWizard()
{
    bool javaRequired = [&]()
    {
        QString currentHostName = QHostInfo::localHostName();
        QString oldHostName = settings()->get("LastHostname").toString();
        if (currentHostName != oldHostName)
        {
            settings()->set("LastHostname", currentHostName);
            return true;
        }
        QString currentJavaPath = settings()->get("JavaPath").toString();
        QString actualPath = FS::ResolveExecutable(currentJavaPath);
        if (actualPath.isNull())
        {
            return true;
        }
        return false;
    }();
    bool analyticsRequired = [&]()
    {
        if(BuildConfig.ANALYTICS_ID.isEmpty())
        {
            return false;
        }
        if (!settings()->get("Analytics").toBool())
        {
            return false;
        }
        if (settings()->get("AnalyticsSeen").toInt() < analytics()->version())
        {
            return true;
        }
        return false;
    }();
    bool languageRequired = [&]()
    {
        if (settings()->get("Language").toString().isEmpty())
            return true;
        return false;
    }();
    bool wizardRequired = javaRequired || analyticsRequired || languageRequired;

    if(wizardRequired)
    {
        m_setupWizard = new SetupWizard(nullptr);
        if (languageRequired)
        {
            m_setupWizard->addPage(new LanguageWizardPage(m_setupWizard));
        }
        if (javaRequired)
        {
            m_setupWizard->addPage(new JavaWizardPage(m_setupWizard));
        }
        if(analyticsRequired)
        {
            m_setupWizard->addPage(new AnalyticsWizardPage(m_setupWizard));
        }
        connect(m_setupWizard, &QDialog::finished, this, &MultiMC::setupWizardFinished);
        m_setupWizard->show();
        return true;
    }
    return false;
}

void MultiMC::setupWizardFinished(int status)
{
    qDebug() << "Wizard result =" << status;
    performMainStartupAction();
}

void MultiMC::performMainStartupAction()
{
    m_status = MultiMC::Initialized;
    if(!m_instanceIdToLaunch.isEmpty())
    {
        auto inst = instances()->getInstanceById(m_instanceIdToLaunch);
        if(inst)
        {
            qDebug() << "<> Instance launching:" << m_instanceIdToLaunch;
            launch(inst, true, nullptr);
            return;
        }
    }
    if(!m_mainWindow)
    {
        // normal main window
        showMainWindow(false);
        qDebug() << "<> Main window shown.";
    }
}

void MultiMC::showFatalErrorMessage(const QString& title, const QString& content)
{
    m_status = MultiMC::Failed;
    auto dialog = CustomMessageBox::selectable(nullptr, title, content, QMessageBox::Critical);
    dialog->exec();
}

MultiMC::~MultiMC()
{
    // kill the other globals.
    Env::dispose();

    // Shut down logger by setting the logger function to nothing
    qInstallMessageHandler(nullptr);

#if defined Q_OS_WIN32
    // Detach from Windows console
    if(consoleAttached)
    {
        fclose(stdout);
        fclose(stdin);
        fclose(stderr);
        FreeConsole();
    }
#endif
}

void MultiMC::messageReceived(const QString& message)
{
    if(status() != Initialized)
    {
        qDebug() << "Received message" << message << "while still initializing. It will be ignored.";
        return;
    }
    if(message == "activate")
    {
        showMainWindow();
    }
    else
    {
        auto inst = instances()->getInstanceById(message);
        if(inst)
        {
            launch(inst, true, nullptr);
        }
    }
}

void MultiMC::analyticsSettingChanged(const Setting&, QVariant value)
{
    if(!m_analytics)
        return;
    bool enabled = value.toBool();
    if(enabled)
    {
        qDebug() << "Analytics enabled by user.";
    }
    else
    {
        qDebug() << "Analytics disabled by user.";
    }
    m_analytics->enable(enabled);
}

std::shared_ptr<TranslationsModel> MultiMC::translations()
{
    return m_translations;
}

std::shared_ptr<JavaInstallList> MultiMC::javalist()
{
    if (!m_javalist)
    {
        m_javalist.reset(new JavaInstallList());
    }
    return m_javalist;
}

std::vector<ITheme *> MultiMC::getValidApplicationThemes()
{
    std::vector<ITheme *> ret;
    auto iter = m_themes.cbegin();
    while (iter != m_themes.cend())
    {
        ret.push_back((*iter).second.get());
        iter++;
    }
    return ret;
}

void MultiMC::setApplicationTheme(const QString& name, bool initial)
{
    auto systemPalette = qApp->palette();
    auto themeIter = m_themes.find(name);
    if(themeIter != m_themes.end())
    {
        auto & theme = (*themeIter).second;
        theme->apply(initial);
    }
    else
    {
        qWarning() << "Tried to set invalid theme:" << name;
    }
}

void MultiMC::setIconTheme(const QString& name)
{
    XdgIcon::setThemeName(name);
}

QIcon MultiMC::getThemedIcon(const QString& name)
{
    return XdgIcon::fromTheme(name);
}

bool MultiMC::openJsonEditor(const QString &filename)
{
    const QString file = QDir::current().absoluteFilePath(filename);
    if (m_settings->get("JsonEditor").toString().isEmpty())
    {
        return DesktopServices::openUrl(QUrl::fromLocalFile(file));
    }
    else
    {
        //return DesktopServices::openFile(m_settings->get("JsonEditor").toString(), file);
        return DesktopServices::run(m_settings->get("JsonEditor").toString(), {file});
    }
}

bool MultiMC::launch(InstancePtr instance, bool online, BaseProfilerFactory *profiler)
{
    if(m_updateRunning)
    {
        qDebug() << "Cannot launch instances while an update is running.";
    }
    else if(instance->canLaunch())
    {
        auto & extras = m_instanceExtras[instance->id()];
        auto & window = extras.window;
        if(window)
        {
            if(!window->saveAll())
            {
                return false;
            }
        }
        auto & controller = extras.controller;
        controller.reset(new LaunchController());
        controller->setInstance(instance);
        controller->setOnline(online);
        controller->setProfiler(profiler);
        if(window)
        {
            controller->setParentWidget(window);
        }
        else if(m_mainWindow)
        {
            controller->setParentWidget(m_mainWindow);
        }
        connect(controller.get(), &LaunchController::succeeded, this, &MultiMC::controllerSucceeded);
        connect(controller.get(), &LaunchController::failed, this, &MultiMC::controllerFailed);
        addRunningInstance();
        controller->start();
        return true;
    }
    else if (instance->isRunning())
    {
        showInstanceWindow(instance, "console");
        return true;
    }
    else if (instance->canEdit())
    {
        showInstanceWindow(instance);
        return true;
    }
    return false;
}

bool MultiMC::kill(InstancePtr instance)
{
    if (!instance->isRunning())
    {
        qWarning() << "Attempted to kill instance" << instance->id() << "which isn't running.";
        return false;
    }
    auto & extras = m_instanceExtras[instance->id()];
    // NOTE: copy of the shared pointer keeps it alive
    auto controller = extras.controller;
    if(controller)
    {
        return controller->abort();
    }
    return true;
}

void MultiMC::addRunningInstance()
{
    m_runningInstances ++;
    if(m_runningInstances == 1)
    {
        emit updateAllowedChanged(false);
    }
}

void MultiMC::subRunningInstance()
{
    if(m_runningInstances == 0)
    {
        qCritical() << "Something went really wrong and we now have less than 0 running instances... WTF";
        return;
    }
    m_runningInstances --;
    if(m_runningInstances == 0)
    {
        emit updateAllowedChanged(true);
    }
}

bool MultiMC::shouldExitNow() const
{
    return m_runningInstances == 0 && m_openWindows == 0;
}

bool MultiMC::updatesAreAllowed()
{
    return m_runningInstances == 0;
}

void MultiMC::updateIsRunning(bool running)
{
    m_updateRunning = running;
}


void MultiMC::controllerSucceeded()
{
    auto controller = qobject_cast<LaunchController *>(QObject::sender());
    if(!controller)
        return;
    auto id = controller->id();
    auto & extras = m_instanceExtras[id];

    // on success, do...
    if (controller->instance()->settings()->get("AutoCloseConsole").toBool())
    {
        if(extras.window)
        {
            extras.window->close();
        }
    }
    extras.controller.reset();
    subRunningInstance();

    // quit when there are no more windows.
    if(shouldExitNow())
    {
        m_status = Status::Succeeded;
        exit(0);
    }
}

void MultiMC::controllerFailed(const QString& error)
{
    Q_UNUSED(error);
    auto controller = qobject_cast<LaunchController *>(QObject::sender());
    if(!controller)
        return;
    auto id = controller->id();
    auto & extras = m_instanceExtras[id];

    // on failure, do... nothing
    extras.controller.reset();
    subRunningInstance();

    // quit when there are no more windows.
    if(shouldExitNow())
    {
        m_status = Status::Failed;
        exit(1);
    }
}

void MultiMC::ShowGlobalSettings(class QWidget* parent, QString open_page)
{
    if(!m_globalSettingsProvider) {
        return;
    }
    emit globalSettingsAboutToOpen();
    {
        SettingsObject::Lock lock(MMC->settings());
        PageDialog dlg(m_globalSettingsProvider.get(), open_page, parent);
        dlg.exec();
    }
    emit globalSettingsClosed();
}

MainWindow* MultiMC::showMainWindow(bool minimized)
{
    if(m_mainWindow)
    {
        m_mainWindow->setWindowState(m_mainWindow->windowState() & ~Qt::WindowMinimized);
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
    else
    {
        m_mainWindow = new MainWindow();
        m_mainWindow->restoreState(QByteArray::fromBase64(MMC->settings()->get("MainWindowState").toByteArray()));
        m_mainWindow->restoreGeometry(QByteArray::fromBase64(MMC->settings()->get("MainWindowGeometry").toByteArray()));
        if(minimized)
        {
            m_mainWindow->showMinimized();
        }
        else
        {
            m_mainWindow->show();
        }

        m_mainWindow->checkInstancePathForProblems();
        connect(this, &MultiMC::updateAllowedChanged, m_mainWindow, &MainWindow::updatesAllowedChanged);
        connect(m_mainWindow, &MainWindow::isClosing, this, &MultiMC::on_windowClose);
        m_openWindows++;
    }
    // FIXME: move this somewhere else...
    if(m_analytics)
    {
        auto windowSize = m_mainWindow->size();
        auto sizeString = QString("%1x%2").arg(windowSize.width()).arg(windowSize.height());
        qDebug() << "Viewport size" << sizeString;
        m_analytics->setViewportSize(sizeString);
        /*
         * cm1 = java min heap [MB]
         * cm2 = java max heap [MB]
         * cm3 = system RAM [MB]
         *
         * cd1 = java version
         * cd2 = java architecture
         * cd3 = system architecture
         * cd4 = CPU architecture
         */
        QVariantMap customValues;
        int min = m_settings->get("MinMemAlloc").toInt();
        int max = m_settings->get("MaxMemAlloc").toInt();
        if(min < max)
        {
            customValues["cm1"] = min;
            customValues["cm2"] = max;
        }
        else
        {
            customValues["cm1"] = max;
            customValues["cm2"] = min;
        }

        constexpr uint64_t Mega = 1024ull * 1024ull;
        int ramSize = int(Sys::getSystemRam() / Mega);
        qDebug() << "RAM size is" << ramSize << "MB";
        customValues["cm3"] = ramSize;

        customValues["cd1"] = m_settings->get("JavaVersion");
        customValues["cd2"] = m_settings->get("JavaArchitecture");
        customValues["cd3"] = Sys::isSystem64bit() ? "64":"32";
        customValues["cd4"] = Sys::isCPU64bit() ? "64":"32";
        auto kernelInfo = Sys::getKernelInfo();
        customValues["cd5"] = kernelInfo.kernelName;
        customValues["cd6"] = kernelInfo.kernelVersion;
        auto distInfo = Sys::getDistributionInfo();
        if(!distInfo.distributionName.isEmpty())
        {
            customValues["cd7"] = distInfo.distributionName;
        }
        if(!distInfo.distributionVersion.isEmpty())
        {
            customValues["cd8"] = distInfo.distributionVersion;
        }
        m_analytics->sendScreenView("Main Window", customValues);
    }
    return m_mainWindow;
}

InstanceWindow *MultiMC::showInstanceWindow(InstancePtr instance, QString page)
{
    if(!instance)
        return nullptr;
    auto id = instance->id();
    auto & extras = m_instanceExtras[id];
    auto & window = extras.window;

    if(window)
    {
        window->raise();
        window->activateWindow();
    }
    else
    {
        window = new InstanceWindow(instance);
        m_openWindows ++;
        connect(window, &InstanceWindow::isClosing, this, &MultiMC::on_windowClose);
    }
    if(!page.isEmpty())
    {
        window->selectPage(page);
    }
    if(extras.controller)
    {
        extras.controller->setParentWidget(window);
    }
    return window;
}

void MultiMC::on_windowClose()
{
    m_openWindows--;
    auto instWindow = qobject_cast<InstanceWindow *>(QObject::sender());
    if(instWindow)
    {
        auto & extras = m_instanceExtras[instWindow->instanceId()];
        extras.window = nullptr;
        if(extras.controller)
        {
            extras.controller->setParentWidget(m_mainWindow);
        }
    }
    auto mainWindow = qobject_cast<MainWindow *>(QObject::sender());
    if(mainWindow)
    {
        m_mainWindow = nullptr;
    }
    // quit when there are no more windows.
    if(shouldExitNow())
    {
        exit(0);
    }
}
