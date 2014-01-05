
#include "MultiMC.h"
#include <iostream>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QStringList>
#include <QDesktopServices>

#include "gui/dialogs/VersionSelectDialog.h"
#include "logic/lists/InstanceList.h"
#include "logic/auth/MojangAccountList.h"
#include "logic/icons/IconList.h"
#include "logic/lists/LwjglVersionList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/lists/ForgeVersionList.h"

#include "logic/news/NewsChecker.h"

#include "logic/InstanceLauncher.h"
#include "logic/net/HttpMetaCache.h"

#include "logic/JavaUtils.h"

#include "logic/updater/UpdateChecker.h"
#include "logic/updater/NotificationChecker.h"

#include "pathutils.h"
#include "cmdutils.h"
#include <inisettingsobject.h>
#include <setting.h>
#include "logger/QsLog.h"
#include <logger/QsLogDest.h>

#include "config.h"
#ifdef WINDOWS
#define UPDATER_BIN "updater.exe"
#elif LINUX
#define UPDATER_BIN "updater"
#elif OSX
#define UPDATER_BIN "updater"
#else
#error Unsupported operating system.
#endif

using namespace Util::Commandline;

MultiMC::MultiMC(int &argc, char **argv, const QString &data_dir_override)
	: QApplication(argc, argv), m_version{VERSION_MAJOR,   VERSION_MINOR,	 VERSION_BUILD,
										  VERSION_CHANNEL, VERSION_BUILD_TYPE}
{
	setOrganizationName("MultiMC");
	setApplicationName("MultiMC5");

	initTranslations();

	setAttribute(Qt::AA_UseHighDpiPixmaps);
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
		parser.addOption("dir", applicationDirPath());
		parser.addShortOpt("dir", 'd');
		parser.addDocumentation("dir", "use the supplied directory as MultiMC root instead of "
									   "the binary location (use '.' for current)");
		// WARNING: disabled until further notice
		/*
		// --launch
		parser.addOption("launch");
		parser.addShortOpt("launch", 'l');
		parser.addDocumentation("launch", "tries to launch the given instance", "<inst>");
*/
		// parse the arguments
		try
		{
			args = parser.parse(arguments());
		}
		catch (ParsingError e)
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
			std::cout << "Version " << VERSION_STR << std::endl;
			std::cout << "Git " << GIT_COMMIT << std::endl;
			m_status = MultiMC::Succeeded;
			return;
		}
	}
	origcwdPath = QDir::currentPath();
	binPath = applicationDirPath();
	QString adjustedBy;
	// change directory
	QString dirParam = args["dir"].toString();
	if (!data_dir_override.isEmpty())
	{
		// the override is used for tests (although dirparam would be enough...)
		// TODO: remove the need for this extra logic
		adjustedBy += "Test override " + data_dir_override;
		dataPath = data_dir_override;
	}
	else if (!dirParam.isEmpty())
	{
		// the dir param. it makes multimc data path point to whatever the user specified
		// on command line
		adjustedBy += "Command line " + dirParam;
		dataPath = dirParam;
	}
	if(!ensureFolderPathExists(dataPath) || !QDir::setCurrent(dataPath))
	{
		// BAD STUFF. WHAT DO?
		initLogger();
		QLOG_ERROR() << "Failed to set work path. Will exit. NOW.";
		m_status = MultiMC::Failed;
		return;
	}

	{
	#ifdef Q_OS_LINUX
		QDir foo(PathCombine(binPath, ".."));
		rootPath = foo.absolutePath();
	#elif defined(Q_OS_WIN32)
		QDir foo(PathCombine(binPath, ".."));
		rootPath = foo.absolutePath();
	#elif defined(Q_OS_MAC)
		QDir foo(PathCombine(binPath, "../.."));
		rootPath = foo.absolutePath();
	#endif
	}

	// init the logger
	initLogger();

	QLOG_INFO() << "MultiMC 5, (c) 2013 MultiMC Contributors";
	QLOG_INFO() << "Version                    : " << VERSION_STR;
	QLOG_INFO() << "Git commit                 : " << GIT_COMMIT;
	if (adjustedBy.size())
	{
		QLOG_INFO() << "Work dir before adjustment : " << origcwdPath;
		QLOG_INFO() << "Work dir after adjustment  : " << QDir::currentPath();
		QLOG_INFO() << "Adjusted by                : " << adjustedBy;
	}
	else
	{
		QLOG_INFO() << "Work dir                   : " << QDir::currentPath();
	}
	QLOG_INFO() << "Binary path                : " << binPath;
	QLOG_INFO() << "Application root path      : " << rootPath;

	// load settings
	initGlobalSettings();

	// initialize the updater
	m_updateChecker.reset(new UpdateChecker());

	// initialize the notification checker
	m_notificationChecker.reset(new NotificationChecker());

	// initialize the news checker
	m_newsChecker.reset(new NewsChecker(NEWS_RSS_URL));

	// and instances
	auto InstDirSetting = m_settings->getSetting("InstanceDir");
	m_instances.reset(new InstanceList(InstDirSetting->get().toString(), this));
	QLOG_INFO() << "Loading Instances...";
	m_instances->loadList();
	connect(InstDirSetting.get(), SIGNAL(settingChanged(const Setting &, QVariant)),
			m_instances.get(), SLOT(on_InstFolderChanged(const Setting &, QVariant)));

	// and accounts
	m_accounts.reset(new MojangAccountList(this));
	QLOG_INFO() << "Loading accounts...";
	m_accounts->setListFilePath("accounts.json", true);
	m_accounts->loadList();

	// init the http meta cache
	initHttpMetaCache();

	// set up a basic autodetected proxy (system default)
	QNetworkProxyFactory::setUseSystemConfiguration(true);

	QLOG_INFO() << "Detecting system proxy settings...";
	auto proxies = QNetworkProxyFactory::systemProxyForQuery();
	if (proxies.size() == 1 && proxies[0].type() == QNetworkProxy::NoProxy)
	{
		QLOG_INFO() << "No proxy found.";
	}
	else
		for (auto proxy : proxies)
		{
			QString proxyDesc;
			if (proxy.type() == QNetworkProxy::NoProxy)
			{
				QLOG_INFO() << "Using no proxy is an option!";
				continue;
			}
			switch (proxy.type())
			{
			case QNetworkProxy::DefaultProxy:
				proxyDesc = "Default proxy: ";
				break;
			case QNetworkProxy::Socks5Proxy:
				proxyDesc = "Socks5 proxy: ";
				break;
			case QNetworkProxy::HttpProxy:
				proxyDesc = "HTTP proxy: ";
				break;
			case QNetworkProxy::HttpCachingProxy:
				proxyDesc = "HTTP caching: ";
				break;
			case QNetworkProxy::FtpCachingProxy:
				proxyDesc = "FTP caching: ";
				break;
			default:
				proxyDesc = "DERP proxy: ";
				break;
			}
			proxyDesc += QString("%3@%1:%2 pass %4")
							 .arg(proxy.hostName())
							 .arg(proxy.port())
							 .arg(proxy.user())
							 .arg(proxy.password());
			QLOG_INFO() << proxyDesc;
		}

	// create the global network manager
	m_qnam.reset(new QNetworkAccessManager(this));

	// launch instance, if that's what should be done
	// WARNING: disabled until further notice
	/*
	if (!args["launch"].isNull())
	{
		if (InstanceLauncher(args["launch"].toString()).launch())
			m_status = MultiMC::Succeeded;
		else
			m_status = MultiMC::Failed;
		return;
	}
*/
	m_status = MultiMC::Initialized;
}

MultiMC::~MultiMC()
{
	if (m_mmc_translator)
	{
		removeTranslator(m_mmc_translator.get());
	}
	if (m_qt_translator)
	{
		removeTranslator(m_qt_translator.get());
	}
}

void MultiMC::initTranslations()
{
	m_qt_translator.reset(new QTranslator());
	if (m_qt_translator->load("qt_" + QLocale::system().name(),
							  QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
	{
		std::cout << "Loading Qt Language File for "
				  << QLocale::system().name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_qt_translator.get()))
		{
			std::cout << " failed.";
			m_qt_translator.reset();
		}
		std::cout << std::endl;
	}
	else
	{
		m_qt_translator.reset();
	}

	m_mmc_translator.reset(new QTranslator());
	if (m_mmc_translator->load("mmc_" + QLocale::system().name(),
							   QDir("translations").absolutePath()))
	{
		std::cout << "Loading MMC Language File for "
				  << QLocale::system().name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_mmc_translator.get()))
		{
			std::cout << " failed.";
			m_mmc_translator.reset();
		}
		std::cout << std::endl;
	}
	else
	{
		m_mmc_translator.reset();
	}
}

void moveFile(const QString &oldName, const QString &newName)
{
	QFile::remove(newName);
	QFile::copy(oldName, newName);
	QFile::remove(oldName);
}
void MultiMC::initLogger()
{
	static const QString logBase = "MultiMC-%0.log";

	moveFile(logBase.arg(3), logBase.arg(4));
	moveFile(logBase.arg(2), logBase.arg(3));
	moveFile(logBase.arg(1), logBase.arg(2));
	moveFile(logBase.arg(0), logBase.arg(1));

	// init the logging mechanism
	QsLogging::Logger &logger = QsLogging::Logger::instance();
	logger.setLoggingLevel(QsLogging::TraceLevel);
	m_fileDestination = QsLogging::DestinationFactory::MakeFileDestination(logBase.arg(0));
	m_debugDestination = QsLogging::DestinationFactory::MakeDebugOutputDestination();
	logger.addDestination(m_fileDestination.get());
	logger.addDestination(m_debugDestination.get());
	// log all the things
	logger.setLoggingLevel(QsLogging::TraceLevel);
}

void MultiMC::initGlobalSettings()
{
	m_settings.reset(new INISettingsObject("multimc.cfg", this));
	// Updates
	m_settings->registerSetting("UseDevBuilds", false);
	m_settings->registerSetting("AutoUpdate", true);
	m_settings->registerSetting("ShownNotifications", QString());

	// FTB
	m_settings->registerSetting("TrackFTBInstances", false);
#ifdef Q_OS_LINUX
	QString ftbDefault = QDir::home().absoluteFilePath(".ftblauncher");
#elif defined(Q_OS_WIN32)
	QString ftbDefault = PathCombine(QDir::homePath(), "AppData/Roaming/ftblauncher");
#elif defined(Q_OS_MAC)
	QString ftbDefault =
		PathCombine(QDir::homePath(), "Library/Application Support/ftblauncher");
#endif
	m_settings->registerSetting("FTBLauncherRoot", ftbDefault);

	m_settings->registerSetting("FTBRoot");
	if (m_settings->get("FTBRoot").isNull())
	{
		QString ftbRoot;
		QFile f(QDir(m_settings->get("FTBLauncherRoot").toString())
					.absoluteFilePath("ftblaunch.cfg"));
		QLOG_INFO() << "Attempting to read" << f.fileName();
		if (f.open(QFile::ReadOnly))
		{
			const QString data = QString::fromLatin1(f.readAll());
			QRegularExpression exp("installPath=(.*)");
			ftbRoot = QDir::cleanPath(exp.match(data).captured(1));
#ifdef Q_OS_WIN32
			if (!ftbRoot.isEmpty())
			{
				if (ftbRoot.at(0).isLetter() && ftbRoot.size() > 1 && ftbRoot.at(1) == '/')
				{
					ftbRoot.remove(1, 1);
				}
			}
#endif
			if (ftbRoot.isEmpty())
			{
				QLOG_INFO() << "Failed to get FTB root path";
			}
			else
			{
				QLOG_INFO() << "FTB is installed at" << ftbRoot;
				m_settings->set("FTBRoot", ftbRoot);
			}
		}
		else
		{
			QLOG_WARN() << "Couldn't open" << f.fileName() << ":" << f.errorString();
			QLOG_WARN() << "This is perfectly normal if you don't have FTB installed";
		}
	}

	// Folders
	m_settings->registerSetting("InstanceDir", "instances");
	m_settings->registerSetting({"CentralModsDir", "ModsDir"}, "mods");
	m_settings->registerSetting({"LWJGLDir", "LwjglDir"}, "lwjgl");
	m_settings->registerSetting("IconsDir", "icons");

	// Editors
	m_settings->registerSetting("JsonEditor", QString());

	// Console
	m_settings->registerSetting("ShowConsole", true);
	m_settings->registerSetting("AutoCloseConsole", true);

	// Console Colors
	//	m_settings->registerSetting("SysMessageColor", QColor(Qt::blue));
	//	m_settings->registerSetting("StdOutColor", QColor(Qt::black));
	//	m_settings->registerSetting("StdErrColor", QColor(Qt::red));

	// Window Size
	m_settings->registerSetting({"LaunchMaximized", "MCWindowMaximize"}, false);
	m_settings->registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
	m_settings->registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

	// Memory
	m_settings->registerSetting({"MinMemAlloc", "MinMemoryAlloc"}, 512);
	m_settings->registerSetting({"MaxMemAlloc", "MaxMemoryAlloc"}, 1024);
	m_settings->registerSetting("PermGen", 64);

	// Java Settings
	m_settings->registerSetting("JavaPath", "");
	m_settings->registerSetting("LastHostname", "");
	m_settings->registerSetting("JvmArgs", "");

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
}

void MultiMC::initHttpMetaCache()
{
	m_metacache.reset(new HttpMetaCache("metacache"));
	m_metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
	m_metacache->addBase("asset_objects", QDir("assets/objects").absolutePath());
	m_metacache->addBase("versions", QDir("versions").absolutePath());
	m_metacache->addBase("libraries", QDir("libraries").absolutePath());
	m_metacache->addBase("minecraftforge", QDir("mods/minecraftforge").absolutePath());
	m_metacache->addBase("skins", QDir("accounts/skins").absolutePath());
	m_metacache->addBase("root", QDir(".").absolutePath());
	m_metacache->Load();
}

std::shared_ptr<IconList> MultiMC::icons()
{
	if (!m_icons)
	{
		m_icons.reset(new IconList);
	}
	return m_icons;
}

std::shared_ptr<LWJGLVersionList> MultiMC::lwjgllist()
{
	if (!m_lwjgllist)
	{
		m_lwjgllist.reset(new LWJGLVersionList());
	}
	return m_lwjgllist;
}

std::shared_ptr<ForgeVersionList> MultiMC::forgelist()
{
	if (!m_forgelist)
	{
		m_forgelist.reset(new ForgeVersionList());
	}
	return m_forgelist;
}

std::shared_ptr<MinecraftVersionList> MultiMC::minecraftlist()
{
	if (!m_minecraftlist)
	{
		m_minecraftlist.reset(new MinecraftVersionList());
	}
	return m_minecraftlist;
}

std::shared_ptr<JavaVersionList> MultiMC::javalist()
{
	if (!m_javalist)
	{
		m_javalist.reset(new JavaVersionList());
	}
	return m_javalist;
}

void MultiMC::installUpdates(const QString &updateFilesDir, bool restartOnFinish)
{
	QLOG_INFO() << "Installing updates.";
#if LINUX
	// On Linux, the MultiMC executable file is actually in the bin folder inside the
	// installation directory.
	// This means that MultiMC's *actual* install path is the parent folder.
	// We need to tell the updater to run with this directory as the install path, rather than
	// the bin folder where the executable is.
	// On other operating systems, we'll just use the path to the executable.
	QString appDir = QFileInfo(MMC->applicationDirPath()).dir().path();

	// On Linux, we also need to set the finish command to the launch script, rather than the
	// binary.
	QString finishCmd = PathCombine(appDir, "MultiMC");
#else
	QString appDir = MMC->applicationDirPath();
	QString finishCmd = MMC->applicationFilePath();
#endif

	// Build the command we'll use to run the updater.
	// Note, the above comment about the app dir path on Linux is irrelevant here because the
	// updater binary is always in the
	// same folder as the main binary.
	QString updaterBinary = PathCombine(MMC->applicationDirPath(), UPDATER_BIN);
	QStringList args;
	// ./updater --install-dir $INSTALL_DIR --package-dir $UPDATEFILES_DIR --script
	// $UPDATEFILES_DIR/file_list.xml --wait $PID --mode main
	args << "--install-dir" << appDir;
	args << "--package-dir" << updateFilesDir;
	args << "--script" << PathCombine(updateFilesDir, "file_list.xml");
	args << "--wait" << QString::number(MMC->applicationPid());

	if (restartOnFinish)
		args << "--finish-cmd" << finishCmd;

	QLOG_INFO() << "Running updater with command" << updaterBinary << args.join(" ");
	QFile::setPermissions(updaterBinary, (QFileDevice::Permission)0x7755);

	if (!QProcess::startDetached(updaterBinary, args))
	{
		QLOG_ERROR() << "Failed to start the updater process!";
		return;
	}

	// Now that we've started the updater, quit MultiMC.
	MMC->quit();
}

void MultiMC::setUpdateOnExit(const QString &updateFilesDir)
{
	m_updateOnExitPath = updateFilesDir;
}

QString MultiMC::getExitUpdatePath() const
{
	return m_updateOnExitPath;
}

bool MultiMC::openJsonEditor(const QString &filename)
{
	const QString file = QDir::current().absoluteFilePath(filename);
	if (m_settings->get("JsonEditor").toString().isEmpty())
	{
		return QDesktopServices::openUrl(QUrl::fromLocalFile(file));
	}
	else
	{
		return QProcess::startDetached(m_settings->get("JsonEditor").toString(), QStringList()
																					 << file);
	}
}

#include "MultiMC.moc"
