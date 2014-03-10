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
#include "logic/lists/LiteLoaderVersionList.h"

#include "logic/news/NewsChecker.h"

#include "logic/status/StatusChecker.h"

#include "logic/InstanceLauncher.h"
#include "logic/net/HttpMetaCache.h"
#include "logic/net/URLConstants.h"

#include "logic/JavaUtils.h"

#include "logic/updater/UpdateChecker.h"
#include "logic/updater/NotificationChecker.h"

#include "logic/tools/JProfiler.h"
#include "logic/tools/JVisualVM.h"
#include "logic/tools/MCEditTool.h"

#include "pathutils.h"
#include "cmdutils.h"
#include <inisettingsobject.h>
#include <setting.h>
#include "logger/QsLog.h"
#include <logger/QsLogDest.h>

#ifdef Q_OS_WIN32
#include <windows.h>
static const int APPDATA_BUFFER_SIZE = 1024;
#endif

using namespace Util::Commandline;

MultiMC::MultiMC(int &argc, char **argv, bool root_override)
	: QApplication(argc, argv),
	  m_version{VERSION_MAJOR,				  VERSION_MINOR,   VERSION_HOTFIX, VERSION_BUILD,
				MultiMCVersion::VERSION_TYPE, VERSION_CHANNEL, BUILD_PLATFORM}
{
	setOrganizationName("MultiMC");
	setApplicationName("MultiMC5");

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
	if (!dirParam.isEmpty())
	{
		// the dir param. it makes multimc data path point to whatever the user specified
		// on command line
		adjustedBy += "Command line " + dirParam;
		dataPath = dirParam;
	}
	else
	{
		dataPath = applicationDirPath();
		adjustedBy += "Fallback to binary path " + dataPath;
	}

	if(!ensureFolderPathExists(dataPath) || !QDir::setCurrent(dataPath))
	{
		// BAD STUFF. WHAT DO?
		initLogger();
		QLOG_ERROR() << "Failed to set work path. Will exit. NOW.";
		m_status = MultiMC::Failed;
		return;
	}

	if (root_override)
	{
		rootPath = binPath;
	}
	else
	{
	#ifdef Q_OS_LINUX
		QDir foo(PathCombine(binPath, ".."));
		rootPath = foo.absolutePath();
	#elif defined(Q_OS_WIN32)
		rootPath = binPath;
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

	// load translations
	initTranslations();

	// initialize the updater
	m_updateChecker.reset(new UpdateChecker());

	// initialize the notification checker
	m_notificationChecker.reset(new NotificationChecker());

	// initialize the news checker
	m_newsChecker.reset(new NewsChecker(NEWS_RSS_URL));

	// initialize the status checker
	m_statusChecker.reset(new StatusChecker());

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

	// create the global network manager
	m_qnam.reset(new QNetworkAccessManager(this));

	// init proxy settings
	updateProxySettings();

	m_profilers.insert("jprofiler",
					   std::shared_ptr<BaseProfilerFactory>(new JProfilerFactory()));
	m_profilers.insert("jvisualvm",
					   std::shared_ptr<BaseProfilerFactory>(new JVisualVMFactory()));
	for (auto profiler : m_profilers.values())
	{
		profiler->registerSettings(m_settings.get());
	}
	m_tools.insert("mcedit",
				   std::shared_ptr<BaseDetachedToolFactory>(new MCEditFactory()));
	for (auto tool : m_tools.values())
	{
		tool->registerSettings(m_settings.get());
	}

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
	connect(this, SIGNAL(aboutToQuit()), SLOT(onExit()));
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
	QLocale locale(m_settings->get("Language").toString());
	QLocale::setDefault(locale);
	QLOG_INFO() << "Your language is" << locale.bcp47Name();
	m_qt_translator.reset(new QTranslator());
	if (m_qt_translator->load("qt_" + locale.bcp47Name(),
							  QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
	{
		QLOG_DEBUG() << "Loading Qt Language File for"
					 << locale.bcp47Name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_qt_translator.get()))
		{
			QLOG_ERROR() << "Loading Qt Language File failed.";
			m_qt_translator.reset();
		}
	}
	else
	{
		m_qt_translator.reset();
	}

	m_mmc_translator.reset(new QTranslator());
	if (m_mmc_translator->load("mmc_" + locale.bcp47Name(), MMC->root() + "/translations"))
	{
		QLOG_DEBUG() << "Loading MMC Language File for"
					 << locale.bcp47Name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_mmc_translator.get()))
		{
			QLOG_ERROR() << "Loading MMC Language File failed.";
			m_mmc_translator.reset();
		}
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
	m_debugDestination = QsLogging::DestinationFactory::MakeQDebugDestination();
	logger.addDestination(m_fileDestination.get());
	logger.addDestination(m_debugDestination.get());
	// log all the things
	logger.setLoggingLevel(QsLogging::TraceLevel);
}

void MultiMC::initGlobalSettings()
{
	m_settings.reset(new INISettingsObject("multimc.cfg", this));
	// Updates
	m_settings->registerSetting("UpdateChannel", version().channel);
	m_settings->registerSetting("AutoUpdate", true);

	// Notifications
	m_settings->registerSetting("ShownNotifications", QString());

	// FTB
	m_settings->registerSetting("TrackFTBInstances", false);
#ifdef Q_OS_LINUX
	QString ftbDefault = QDir::home().absoluteFilePath(".ftblauncher");
#elif defined(Q_OS_WIN32)
	wchar_t buf[APPDATA_BUFFER_SIZE];
	QString ftbDefault;
	if(!GetEnvironmentVariableW(L"APPDATA", buf, APPDATA_BUFFER_SIZE))
	{
		QLOG_FATAL() << "Your APPDATA folder is missing! If you are on windows, this means your system is broken. If you aren't on windows, how the **** are you running the windows build????";
	}
	else
	{
		ftbDefault = PathCombine(QString::fromWCharArray(buf), "ftblauncher");
	}
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

	// Language
	m_settings->registerSetting("Language", QLocale(QLocale::system().language()).bcp47Name());

	// Console
	m_settings->registerSetting("ShowConsole", true);
	m_settings->registerSetting("AutoCloseConsole", true);
	m_settings->registerSetting("LogPrePostOutput", true);

	// Console Colors
	//	m_settings->registerSetting("SysMessageColor", QColor(Qt::blue));
	//	m_settings->registerSetting("StdOutColor", QColor(Qt::black));
	//	m_settings->registerSetting("StdErrColor", QColor(Qt::red));

	// Window Size
	m_settings->registerSetting({"LaunchMaximized", "MCWindowMaximize"}, false);
	m_settings->registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
	m_settings->registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

	// Proxy Settings
	m_settings->registerSetting("ProxyType", "Default");
	m_settings->registerSetting({"ProxyAddr", "ProxyHostName"}, "127.0.0.1");
	m_settings->registerSetting("ProxyPort", 8080);
	m_settings->registerSetting({"ProxyUser", "ProxyUsername"}, "");
	m_settings->registerSetting({"ProxyPass", "ProxyPassword"}, "");

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
	m_metacache->addBase("liteloader", QDir("mods/liteloader").absolutePath());
	m_metacache->addBase("skins", QDir("accounts/skins").absolutePath());
	m_metacache->addBase("root", QDir(root()).absolutePath());
	m_metacache->Load();
}

void MultiMC::updateProxySettings()
{
	QString proxyTypeStr = settings()->get("ProxyType").toString();

	// Get the proxy settings from the settings object.
	QString addr = settings()->get("ProxyAddr").toString();
	int port = settings()->get("ProxyPort").value<qint16>();
	QString user = settings()->get("ProxyUser").toString();
	QString pass = settings()->get("ProxyPass").toString();

	// Set the application proxy settings.
	if (proxyTypeStr == "SOCKS5")
	{
		QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, addr, port, user, pass));
	}
	else if (proxyTypeStr == "HTTP")
	{
		QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, addr, port, user, pass));
	}
	else if (proxyTypeStr == "None")
	{
		// If we have no proxy set, set no proxy and return.
		QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
	}
	else
	{
		// If we have "Default" selected, set Qt to use the system proxy settings.
		QNetworkProxyFactory::setUseSystemConfiguration(true);
	}

	QLOG_INFO() << "Detecting proxy settings...";
	QNetworkProxy proxy = QNetworkProxy::applicationProxy();
	if (m_qnam.get()) m_qnam->setProxy(proxy);
	QString proxyDesc;
	if (proxy.type() == QNetworkProxy::NoProxy)
	{
		QLOG_INFO() << "Using no proxy is an option!";
		return;
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

std::shared_ptr<LiteLoaderVersionList> MultiMC::liteloaderlist()
{
	if (!m_liteloaderlist)
	{
		m_liteloaderlist.reset(new LiteLoaderVersionList());
	}
	return m_liteloaderlist;
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

void MultiMC::installUpdates(const QString updateFilesDir, UpdateFlags flags)
{
	// if we are going to update on exit, save the params now
	if(flags & OnExit)
	{
		m_updateOnExitPath = updateFilesDir;
		m_updateOnExitFlags = flags & ~OnExit;
		return;
	}
	// otherwise if there already were some params for on exit update, clear them and continue
	else if(m_updateOnExitPath.size())
	{
		m_updateOnExitFlags = None;
		m_updateOnExitPath.clear();
	}
	QLOG_INFO() << "Installing updates.";
	#ifdef WINDOWS
		QString finishCmd = MMC->applicationFilePath();
		QString updaterBinary = PathCombine(bin(), "updater.exe");
	#elif LINUX
		QString finishCmd = PathCombine(root(), "MultiMC");
		QString updaterBinary = PathCombine(bin(), "updater");
	#elif OSX
		QString finishCmd = MMC->applicationFilePath();
		QString updaterBinary = PathCombine(bin(), "updater");
	#else
		#error Unsupported operating system.
	#endif

	QStringList args;
	// ./updater --install-dir $INSTALL_DIR --package-dir $UPDATEFILES_DIR --script
	// $UPDATEFILES_DIR/file_list.xml --wait $PID --mode main
	args << "--install-dir" << root();
	args << "--package-dir" << updateFilesDir;
	args << "--script" << PathCombine(updateFilesDir, "file_list.xml");
	args << "--wait" << QString::number(MMC->applicationPid());
	if(flags & DryRun)
		args << "--dry-run";
	if (flags & RestartOnFinish)
	{
		args << "--finish-cmd" << finishCmd;
		args << "--finish-dir" << data();
	}
	QLOG_INFO() << "Running updater with command" << updaterBinary << args.join(" ");
	QFile::setPermissions(updaterBinary, (QFileDevice::Permission)0x7755);

	if (!QProcess::startDetached(updaterBinary, args/*, root()*/))
	{
		QLOG_ERROR() << "Failed to start the updater process!";
		return;
	}

	// Now that we've started the updater, quit MultiMC.
	MMC->quit();
}

void MultiMC::onExit()
{
	if(m_updateOnExitPath.size())
	{
		installUpdates(m_updateOnExitPath, m_updateOnExitFlags);
	}
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
