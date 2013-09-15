
#include "MultiMC.h"
#include <iostream>
#include <QDir>
#include <QNetworkAccessManager>
#include <QTranslator>
#include <QLibraryInfo>

#include "gui/mainwindow.h"
#include "logic/lists/InstanceList.h"
#include "logic/lists/IconList.h"
#include "logic/lists/LwjglVersionList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/lists/ForgeVersionList.h"

#include "logic/InstanceLauncher.h"
#include "logic/net/HttpMetaCache.h"


#include "pathutils.h"
#include "cmdutils.h"
#include <inisettingsobject.h>
#include <setting.h>

#include "config.h"
using namespace Util::Commandline;

MultiMC::MultiMC ( int& argc, char** argv )
	:QApplication ( argc, argv )
{
	setOrganizationName("Forkk");
	setApplicationName("MultiMC 5");
	
	initTranslations();
	
	// Print app header
	std::cout << "MultiMC 5" << std::endl;
	std::cout << "(c) 2013 MultiMC Contributors" << std::endl << std::endl;
	
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
		parser.addOption("dir", applicationDirPath());
		parser.addShortOpt("dir", 'd');
		parser.addDocumentation("dir", "use the supplied directory as MultiMC root instead of the binary location (use '.' for current)");
		// --update
		parser.addOption("update");
		parser.addShortOpt("update", 'u');
		parser.addDocumentation("update", "replaces the given file with the running executable", "<path>");
		// --quietupdate
		parser.addSwitch("quietupdate");
		parser.addShortOpt("quietupdate", 'U');
		parser.addDocumentation("quietupdate", "doesn't restart MultiMC after installing updates");
		// --launch
		parser.addOption("launch");
		parser.addShortOpt("launch", 'l');
		parser.addDocumentation("launch", "tries to launch the given instance", "<inst>");
		
		// parse the arguments
		try
		{
			args = parser.parse(arguments());
		}
		catch(ParsingError e)
		{
			std::cerr << "CommandLineError: " << e.what() << std::endl;
			std::cerr << "Try '%1 -h' to get help on MultiMC's command line parameters." << std::endl;
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
			std::cout << "Version " << VERSION_STR << std::endl;
			std::cout << "Git " << GIT_COMMIT << std::endl;
			std::cout << "Tag: " << JENKINS_BUILD_TAG << " " << (ARCH==x64?"x86_64":"x86") << std::endl;
			m_status = MultiMC::Succeeded;
			return;
		}
		
		// update
		// Note: cwd is always the current executable path!
		if (!args["update"].isNull())
		{
			std::cout << "Performing MultiMC update: " << qPrintable(args["update"].toString()) << std::endl;
			QString cwd = QDir::currentPath();
			QDir::setCurrent(applicationDirPath());
			QFile file(applicationFilePath());
			file.copy(args["update"].toString());
			if(args["quietupdate"].toBool())
			{
				m_status = MultiMC::Succeeded;
				return;
			}
			QDir::setCurrent(cwd);
		}
	}
	
	// change directory
	QDir::setCurrent(args["dir"].toString());
	
	// load settings
	initGlobalSettings();
	
	// and instances
	m_instances = new InstanceList(m_settings->get("InstanceDir").toString(),this);
	std::cout << "Loading Instances..." << std::endl;
	m_instances->loadList();
	
	// init the http meta cache
	initHttpMetaCache();
	
	// create the global network manager
	m_qnam = new QNetworkAccessManager(this);
	
	// Register meta types.
	qRegisterMetaType<LoginResponse>("LoginResponse");
	
	// launch instance, if that's what should be done
	if (!args["launch"].isNull())
	{
		if(InstanceLauncher(args["launch"].toString()).launch())
			m_status = MultiMC::Succeeded;
		else
			m_status = MultiMC::Failed;
		return;
	}
	m_status = MultiMC::Initialized;
}

MultiMC::~MultiMC()
{
	if(m_mmc_translator)
	{
		removeTranslator(m_mmc_translator);
		delete m_mmc_translator;
		m_mmc_translator = nullptr;
	}
	if(m_qt_translator)
	{
		removeTranslator(m_qt_translator);
		delete m_qt_translator;
		m_qt_translator = nullptr;
	}
	if(m_icons)
	{
		delete m_icons;
		m_icons = nullptr;
	}
	if(m_lwjgllist)
	{
		delete m_lwjgllist;
		m_lwjgllist = nullptr;
	}
	if(m_minecraftlist)
	{
		delete m_minecraftlist;
		m_minecraftlist = nullptr;
	}
	delete m_settings;
	delete m_metacache;
}

void MultiMC::initTranslations()
{
	m_qt_translator = new QTranslator();
	if(m_qt_translator->load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
	{
		std::cout
			<< "Loading Qt Language File for "
			<< QLocale::system().name().toLocal8Bit().constData()
			<< "...";
		if(!installTranslator(m_qt_translator))
		{
			std::cout << " failed.";
		}
		std::cout << std::endl;
	}
	else
	{
		delete m_qt_translator;
		m_qt_translator = nullptr;
	}

	m_mmc_translator = new QTranslator();
	if(m_mmc_translator->load("mmc_" + QLocale::system().name(), QDir("translations").absolutePath()))
	{
		std::cout
			<< "Loading MMC Language File for "
			<< QLocale::system().name().toLocal8Bit().constData()
			<< "...";
		if(!installTranslator(m_mmc_translator))
		{
			std::cout << " failed.";
		}
		std::cout << std::endl;
	}
	else
	{
		delete m_mmc_translator;
		m_mmc_translator = nullptr;
	}
}


void MultiMC::initGlobalSettings()
{
	m_settings = new INISettingsObject("multimc.cfg", this);
		// Updates
	m_settings->registerSetting(new Setting("UseDevBuilds", false));
	m_settings->registerSetting(new Setting("AutoUpdate", true));
	
	// Folders
	m_settings->registerSetting(new Setting("InstanceDir", "instances"));
	m_settings->registerSetting(new Setting("CentralModsDir", "mods"));
	m_settings->registerSetting(new Setting("LWJGLDir", "lwjgl"));
	
	// Console
	m_settings->registerSetting(new Setting("ShowConsole", true));
	m_settings->registerSetting(new Setting("AutoCloseConsole", true));
	
	// Toolbar settings
	m_settings->registerSetting(new Setting("InstanceToolbarVisible", true));
	m_settings->registerSetting(new Setting("InstanceToolbarPosition", QPoint()));
	
	// Console Colors
//	m_settings->registerSetting(new Setting("SysMessageColor", QColor(Qt::blue)));
//	m_settings->registerSetting(new Setting("StdOutColor", QColor(Qt::black)));
//	m_settings->registerSetting(new Setting("StdErrColor", QColor(Qt::red)));
	
	// Window Size
	m_settings->registerSetting(new Setting("LaunchMaximized", false));
	m_settings->registerSetting(new Setting("MinecraftWinWidth", 854));
	m_settings->registerSetting(new Setting("MinecraftWinHeight", 480));
	
	// Auto login
	m_settings->registerSetting(new Setting("AutoLogin", false));
	
	// Memory
	m_settings->registerSetting(new Setting("MinMemAlloc", 512));
	m_settings->registerSetting(new Setting("MaxMemAlloc", 1024));
	m_settings->registerSetting(new Setting("PermGen", 64));
	
	// Java Settings
	m_settings->registerSetting(new Setting("JavaPath", "java"));
	m_settings->registerSetting(new Setting("JvmArgs", ""));
	
	// Custom Commands
	m_settings->registerSetting(new Setting("PreLaunchCommand", ""));
	m_settings->registerSetting(new Setting("PostExitCommand", ""));
	
	// The cat
	m_settings->registerSetting(new Setting("TheCat", false));
	
	// Shall the main window hide on instance launch
	m_settings->registerSetting(new Setting("NoHide", false));
}

void MultiMC::initHttpMetaCache()
{
	m_metacache = new HttpMetaCache("metacache");
	m_metacache->addBase("assets", QDir("assets").absolutePath());
	m_metacache->addBase("versions", QDir("versions").absolutePath());
	m_metacache->addBase("libraries", QDir("libraries").absolutePath());
	m_metacache->Load();
}


IconList* MultiMC::icons()
{
	if ( !m_icons )
	{
		m_icons = new IconList;
	}
	return m_icons;
}

LWJGLVersionList* MultiMC::lwjgllist()
{
	if ( !m_lwjgllist )
	{
		m_lwjgllist = new LWJGLVersionList();
	}
	return m_lwjgllist;
}
ForgeVersionList* MultiMC::forgelist()
{
	if ( !m_forgelist )
	{
		m_forgelist = new ForgeVersionList();
	}
	return m_forgelist;
}

MinecraftVersionList* MultiMC::minecraftlist()
{
	if ( !m_minecraftlist )
	{
		m_minecraftlist = new MinecraftVersionList();
	}
	return m_minecraftlist;
}


int main(int argc, char *argv[])
{
	// initialize Qt
	MultiMC app(argc, argv);
	
	// show main window
	MainWindow mainWin;
	mainWin.show();
	
	
	
	switch(app.status())
	{
		case MultiMC::Initialized:
			return app.exec();
		case MultiMC::Failed:
			return 1;
		case MultiMC::Succeeded:
			return 0;
	}
}

#include "MultiMC.moc"