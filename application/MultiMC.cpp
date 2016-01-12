#include "MultiMC.h"
#include "BuildConfig.h"
#include "pages/BasePageProvider.h"
#include "pages/global/MultiMCPage.h"
#include "pages/global/MinecraftPage.h"
#include "pages/global/JavaPage.h"
#include "pages/global/ProxyPage.h"
#include "pages/global/ExternalToolsPage.h"
#include "pages/global/AccountListPage.h"
#include "pages/global/PasteEEPage.h"

#include <iostream>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QStringList>
#include <QDebug>

#include "InstanceList.h"
#include "auth/MojangAccountList.h"
#include "icons/IconList.h"
#include "minecraft/LwjglVersionList.h"
#include "minecraft/MinecraftVersionList.h"
#include "liteloader/LiteLoaderVersionList.h"

#include "forge/ForgeVersionList.h"

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

#include "trans/TranslationDownloader.h"
#include "resources/Resource.h"
#include "handlers/IconResourceHandler.h"
#include "handlers/WebResourceHandler.h"

#include "ftb/FTBPlugin.h"

#include <Commandline.h>
#include <FileSystem.h>
#include <DesktopServices.h>

using namespace Commandline;

MultiMC::MultiMC(int &argc, char **argv, bool test_mode) : QApplication(argc, argv)
{
	setOrganizationName("MultiMC");
	setApplicationName("MultiMC5");

	startTime = QDateTime::currentDateTime();

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
		// --launch
		parser.addOption("launch");
		parser.addShortOpt("launch", 'l');
		parser.addDocumentation("launch", "launch the specified instance (by instance ID)");

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
			std::cout << "Version " << BuildConfig.printableVersionString().toStdString() << std::endl;
			std::cout << "Git " << BuildConfig.GIT_COMMIT.toStdString() << std::endl;
			m_status = MultiMC::Succeeded;
			return;
		}
	}

	QString origcwdPath = QDir::currentPath();
	QString binPath = applicationDirPath();
	QString adjustedBy;
	QString dataPath;
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

	launchId = args["launch"].toString();

	if (!FS::ensureFolderPathExists(dataPath) || !QDir::setCurrent(dataPath))
	{
		// BAD STUFF. WHAT DO?
		initLogger();
		qCritical() << "Failed to set work path. Will exit. NOW.";
		m_status = MultiMC::Failed;
		return;
	}

	// in test mode, root path is the same as the binary path.
	if (test_mode)
	{
		rootPath = binPath;
	}
	else
	{
#ifdef Q_OS_LINUX
		QDir foo(FS::PathCombine(binPath, ".."));
		rootPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
		rootPath = binPath;
#elif defined(Q_OS_MAC)
		QDir foo(FS::PathCombine(binPath, "../.."));
		rootPath = foo.absolutePath();
#endif
	}

	// init the logger
	initLogger();

	qDebug() << "MultiMC 5, (c) 2013-2015 MultiMC Contributors";
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
	qDebug() << "Application root path      : " << rootPath;

	// load settings
	initGlobalSettings(test_mode);

	// load translations
	initTranslations();

	// initialize the updater
	if(BuildConfig.UPDATER_ENABLED)
	{
		m_updateChecker.reset(new UpdateChecker(BuildConfig.CHANLIST_URL, BuildConfig.VERSION_CHANNEL, BuildConfig.VERSION_BUILD));
	}

	m_translationChecker.reset(new TranslationDownloader());

	// load icons
	initIcons();

	// and instances
	auto InstDirSetting = m_settings->getSetting("InstanceDir");
	// instance path: check for problems with '!' in instance path and warn the user in the log
	// and rememer that we have to show him a dialog when the gui starts (if it does so)
	QString instDir = m_settings->get("InstanceDir").toString();
	qDebug() << "Instance path              : " << instDir;
	if (FS::checkProblemticPathJava(QDir(instDir)))
	{
		qWarning()
			<< "Your instance path contains \'!\' and this is known to cause java problems";
	}
	m_instances.reset(new InstanceList(m_settings, InstDirSetting->get().toString(), this));
	qDebug() << "Loading Instances...";
	m_instances->loadList();
	connect(InstDirSetting.get(), SIGNAL(SettingChanged(const Setting &, QVariant)),
			m_instances.get(), SLOT(on_InstFolderChanged(const Setting &, QVariant)));

	// and accounts
	m_accounts.reset(new MojangAccountList(this));
	qDebug() << "Loading accounts...";
	m_accounts->setListFilePath("accounts.json", true);
	m_accounts->loadList();

	// init the http meta cache
	ENV.initHttpMetaCache();

	// create the global network manager
	ENV.m_qnam.reset(new QNetworkAccessManager(this));

	// init proxy settings
	{
		QString proxyTypeStr = settings()->get("ProxyType").toString();
		QString addr = settings()->get("ProxyAddr").toString();
		int port = settings()->get("ProxyPort").value<qint16>();
		QString user = settings()->get("ProxyUser").toString();
		QString pass = settings()->get("ProxyPass").toString();
		ENV.updateProxySettings(proxyTypeStr, addr, port, user, pass);
	}

	initSSL();

	m_translationChecker->downloadTranslations();

	//FIXME: what to do with these?
	m_profilers.insert("jprofiler",
					   std::shared_ptr<BaseProfilerFactory>(new JProfilerFactory()));
	m_profilers.insert("jvisualvm",
					   std::shared_ptr<BaseProfilerFactory>(new JVisualVMFactory()));
	for (auto profiler : m_profilers.values())
	{
		profiler->registerSettings(m_settings);
	}

	//FIXME: what to do with these?
	m_tools.insert("mcedit", std::shared_ptr<BaseDetachedToolFactory>(new MCEditFactory()));
	for (auto tool : m_tools.values())
	{
		tool->registerSettings(m_settings);
	}

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

#ifdef Q_OS_MAC
#include "CertWorkaround.h"
#endif

void MultiMC::initSSL()
{
#ifdef Q_OS_MAC
	Q_INIT_RESOURCE(certs);
	RebuildQtCertificates();
	QFile equifaxFile(":/certs/Equifax_Secure_Certificate_Authority.pem");
	equifaxFile.open(QIODevice::ReadOnly);
	QSslCertificate equifaxCert(equifaxFile.readAll(), QSsl::Pem);
	QSslSocket::addDefaultCaCertificate(equifaxCert);
#endif
}

void MultiMC::initTranslations()
{
	QLocale locale(m_settings->get("Language").toString());
	QLocale::setDefault(locale);
	qDebug() << "Your language is" << locale.bcp47Name();
	m_qt_translator.reset(new QTranslator());
	if (m_qt_translator->load("qt_" + locale.bcp47Name(),
							  QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
	{
		qDebug() << "Loading Qt Language File for"
					 << locale.bcp47Name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_qt_translator.get()))
		{
			qCritical() << "Loading Qt Language File failed.";
			m_qt_translator.reset();
		}
	}
	else
	{
		m_qt_translator.reset();
	}

	m_mmc_translator.reset(new QTranslator());
	if (m_mmc_translator->load("mmc_" + locale.bcp47Name(), FS::PathCombine(QDir::currentPath(), "translations")))
	{
		qDebug() << "Loading MMC Language File for"
					 << locale.bcp47Name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_mmc_translator.get()))
		{
			qCritical() << "Loading MMC Language File failed.";
			m_mmc_translator.reset();
		}
	}
	else
	{
		m_mmc_translator.reset();
	}
}

void MultiMC::initIcons()
{
	auto setting = MMC->settings()->getSetting("IconsDir");
	ENV.m_icons.reset(new IconList(QString(":/icons/instances/"), setting->get().toString()));
	connect(setting.get(), &Setting::SettingChanged,[&](const Setting &, QVariant value)
	{
		ENV.m_icons->directoryChanged(value.toString());
	});

	//FIXME: none of this should be here.
	Resource::registerHandler<WebResourceHandler>("web");
	Resource::registerHandler<IconResourceHandler>("icon");

	Resource::registerTransformer([](const QByteArray &data) -> QPixmap
	{
		return QPixmap::fromImage(QImage::fromData(data));
	});
	Resource::registerTransformer([](const QByteArray &data) -> QIcon
	{
		return QIcon(QPixmap::fromImage(QImage::fromData(data)));
	});
}


void moveFile(const QString &oldName, const QString &newName)
{
	QFile::remove(newName);
	QFile::copy(oldName, newName);
	QFile::remove(oldName);
}


void appDebugOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	const char *levels = "DWCF";
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

void MultiMC::initLogger()
{
	static const QString logBase = "MultiMC-%0.log";

	moveFile(logBase.arg(3), logBase.arg(4));
	moveFile(logBase.arg(2), logBase.arg(3));
	moveFile(logBase.arg(1), logBase.arg(2));
	moveFile(logBase.arg(0), logBase.arg(1));

	qInstallMessageHandler(appDebugOutput);

	logFile = std::make_shared<QFile>(logBase.arg(0));
	logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
}

void MultiMC::initGlobalSettings(bool test_mode)
{
	m_settings.reset(new INISettingsObject("multimc.cfg", this));
	// Updates
	m_settings->registerSetting("UpdateChannel", BuildConfig.VERSION_CHANNEL);
	m_settings->registerSetting("AutoUpdate", true);
	m_settings->registerSetting("IconTheme", QString("multimc"));

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
	if(!test_mode)
	{
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
	}
	else
	{
		// in test mode, we don't have UI, so we don't do any font resolving
		m_settings->registerSetting("ConsoleFont", defaultMonospace);
	}
	m_settings->registerSetting("ConsoleFontSize", defaultSize);
	m_settings->registerSetting("ConsoleMaxLines", 100000);
	m_settings->registerSetting("ConsoleOverflowStop", true);

	FTBPlugin::initialize(m_settings);

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
	m_settings->registerSetting("RaiseConsole", true);
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
	m_settings->registerSetting("JavaVersion", "");
	m_settings->registerSetting("LastHostname", "");
	m_settings->registerSetting("JavaDetectionHack", "");
	m_settings->registerSetting("JvmArgs", "");

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

	// Jar mod nag dialog in version page
	m_settings->registerSetting("JarModNagSeen", false);

	// paste.ee API key
	m_settings->registerSetting("PasteEEAPIKey", "multimc");

	// Init page provider
	{
		m_globalSettingsProvider = std::make_shared<GenericPageProvider>(tr("Settings"));
		m_globalSettingsProvider->addPage<MultiMCPage>();
		m_globalSettingsProvider->addPage<MinecraftPage>();
		m_globalSettingsProvider->addPage<JavaPage>();
		m_globalSettingsProvider->addPage<ProxyPage>();
		m_globalSettingsProvider->addPage<ExternalToolsPage>();
		m_globalSettingsProvider->addPage<AccountListPage>();
		m_globalSettingsProvider->addPage<PasteEEPage>();
	}
}

std::shared_ptr<LWJGLVersionList> MultiMC::lwjgllist()
{
	if (!m_lwjgllist)
	{
		m_lwjgllist.reset(new LWJGLVersionList());
		ENV.registerVersionList("org.lwjgl.legacy", m_lwjgllist);
	}
	return m_lwjgllist;
}

std::shared_ptr<ForgeVersionList> MultiMC::forgelist()
{
	if (!m_forgelist)
	{
		m_forgelist.reset(new ForgeVersionList());
		ENV.registerVersionList("net.minecraftforge", m_forgelist);
	}
	return m_forgelist;
}

std::shared_ptr<LiteLoaderVersionList> MultiMC::liteloaderlist()
{
	if (!m_liteloaderlist)
	{
		m_liteloaderlist.reset(new LiteLoaderVersionList());
		ENV.registerVersionList("com.mumfrey.liteloader", m_liteloaderlist);
	}
	return m_liteloaderlist;
}

std::shared_ptr<MinecraftVersionList> MultiMC::minecraftlist()
{
	if (!m_minecraftlist)
	{
		m_minecraftlist.reset(new MinecraftVersionList());
		ENV.registerVersionList("net.minecraft", m_minecraftlist);
	}
	return m_minecraftlist;
}

std::shared_ptr<JavaInstallList> MultiMC::javalist()
{
	if (!m_javalist)
	{
		m_javalist.reset(new JavaInstallList());
		ENV.registerVersionList("com.java", m_javalist);
	}
	return m_javalist;
}

// from <sys/stat.h>
#ifndef S_IRUSR
#define __S_IREAD 0400         /* Read by owner.  */
#define __S_IWRITE 0200        /* Write by owner.  */
#define __S_IEXEC 0100         /* Execute by owner.  */
#define S_IRUSR __S_IREAD      /* Read by owner.  */
#define S_IWUSR __S_IWRITE     /* Write by owner.  */
#define S_IXUSR __S_IEXEC      /* Execute by owner.  */

#define S_IRGRP (S_IRUSR >> 3) /* Read by group.  */
#define S_IWGRP (S_IWUSR >> 3) /* Write by group.  */
#define S_IXGRP (S_IXUSR >> 3) /* Execute by group.  */

#define S_IROTH (S_IRGRP >> 3) /* Read by others.  */
#define S_IWOTH (S_IWGRP >> 3) /* Write by others.  */
#define S_IXOTH (S_IXGRP >> 3) /* Execute by others.  */
#endif
static QFile::Permissions unixModeToPermissions(const int mode)
{
	QFile::Permissions perms;

	if (mode & S_IRUSR)
	{
		perms |= QFile::ReadUser;
	}
	if (mode & S_IWUSR)
	{
		perms |= QFile::WriteUser;
	}
	if (mode & S_IXUSR)
	{
		perms |= QFile::ExeUser;
	}

	if (mode & S_IRGRP)
	{
		perms |= QFile::ReadGroup;
	}
	if (mode & S_IWGRP)
	{
		perms |= QFile::WriteGroup;
	}
	if (mode & S_IXGRP)
	{
		perms |= QFile::ExeGroup;
	}

	if (mode & S_IROTH)
	{
		perms |= QFile::ReadOther;
	}
	if (mode & S_IWOTH)
	{
		perms |= QFile::WriteOther;
	}
	if (mode & S_IXOTH)
	{
		perms |= QFile::ExeOther;
	}
	return perms;
}

void MultiMC::installUpdates(const QString updateFilesDir, GoUpdate::OperationList operations)
{
	qint64 pid = -1;
	QStringList args;
	bool started = false;

	qDebug() << "Installing updates.";
#ifdef Q_OS_WIN
	QString finishCmd = applicationFilePath();
#elif defined Q_OS_LINUX
	QString finishCmd = FS::PathCombine(root(), "MultiMC");
#elif defined Q_OS_MAC
	QString finishCmd = applicationFilePath();
#else
#error Unsupported operating system.
#endif

	QString backupPath = FS::PathCombine(root(), "update", "backup");
	QDir origin(root());

	// clean up the backup folder. it should be empty before we start
	if(!FS::deletePath(backupPath))
	{
		qWarning() << "couldn't remove previous backup folder" << backupPath;
	}
	// and it should exist.
	if(!FS::ensureFolderPathExists(backupPath))
	{
		qWarning() << "couldn't create folder" << backupPath;
		return;
	}

	struct BackupEntry
	{
		QString orig;
		QString backup;
	};
	enum Failure
	{
		Replace,
		Delete,
		Start,
		Nothing
	} failedOperationType = Nothing;
	QString failedFile;

	QList <BackupEntry> backups;
	QList <BackupEntry> trashcan;

	bool useXPHack = false;
	QString exePath;
	QString exeOrigin;
	QString exeBackup;

	// perform the update operations
	for(auto op: operations)
	{
		switch(op.type)
		{
			// replace = move original out to backup, if it exists, move the new file in its place
			case GoUpdate::Operation::OP_REPLACE:
			{
#ifdef Q_OS_WIN32
				// hack for people renaming the .exe because ... reasons :)
				if(op.dest == "MultiMC.exe")
				{
					op.dest = QFileInfo(applicationFilePath()).fileName();
				}
#endif
				QFileInfo replaced (FS::PathCombine(root(), op.dest));
#ifdef Q_OS_WIN32
				if(QSysInfo::windowsVersion() < QSysInfo::WV_VISTA)
				{
					if(replaced.fileName() == "MultiMC.exe")
					{
						QDir rootDir(root());
						exeOrigin = rootDir.relativeFilePath(op.file);
						exePath = rootDir.relativeFilePath(op.dest);
						exeBackup = rootDir.relativeFilePath(FS::PathCombine(backupPath, replaced.fileName()));
						useXPHack = true;
						continue;
					}
				}
#endif
				if(replaced.exists())
				{
					QString backupName = op.dest;
					backupName.replace('/', '_');
					QString backupFilePath = FS::PathCombine(backupPath, backupName);
					if(!QFile::rename(replaced.absoluteFilePath(), backupFilePath))
					{
						qWarning() << "Couldn't move:" << replaced.absoluteFilePath() << "to" << backupFilePath;
						failedOperationType = Replace;
						failedFile = op.dest;
						goto FAILED;
					}
					BackupEntry be;
					be.orig = replaced.absoluteFilePath();
					be.backup = backupFilePath;
					backups.append(be);
				}
				// make sure the folder we are putting this into exists
				if(!FS::ensureFilePathExists(replaced.absoluteFilePath()))
				{
					qWarning() << "REPLACE: Couldn't create folder:" << replaced.absoluteFilePath();
					failedOperationType = Replace;
					failedFile = op.dest;
					goto FAILED;
				}
				// now move the new file in
				if(!QFile::rename(op.file, replaced.absoluteFilePath()))
				{
					qWarning() << "REPLACE: Couldn't move:" << op.file << "to" << replaced.absoluteFilePath();
					failedOperationType = Replace;
					failedFile = op.dest;
					goto FAILED;
				}
				QFile::setPermissions(replaced.absoluteFilePath(), unixModeToPermissions(op.mode));
			}
			break;
			// delete = move original to backup
			case GoUpdate::Operation::OP_DELETE:
			{
				QString origFilePath = FS::PathCombine(root(), op.file);
				if(QFile::exists(origFilePath))
				{
					QString backupName = op.file;
					backupName.replace('/', '_');
					QString trashFilePath = FS::PathCombine(backupPath, backupName);

					if(!QFile::rename(origFilePath, trashFilePath))
					{
						qWarning() << "DELETE: Couldn't move:" << op.file << "to" << trashFilePath;
						failedFile = op.file;
						failedOperationType = Delete;
						goto FAILED;
					}
					BackupEntry be;
					be.orig = origFilePath;
					be.backup = trashFilePath;
					trashcan.append(be);
				}
			}
			break;
		}
	}

	// try to start the new binary
	args = qApp->arguments();
	args.removeFirst();

	// on old Windows, do insane things... no error checking here, this is just to have something.
	if(useXPHack)
	{
		QString script;
		auto nativePath = QDir::toNativeSeparators(exePath);
		auto nativeOriginPath = QDir::toNativeSeparators(exeOrigin);
		auto nativeBackupPath = QDir::toNativeSeparators(exeBackup);

		// so we write this vbscript thing...
		QTextStream out(&script);
		out << "WScript.Sleep 1000\n";
		out << "Set fso=CreateObject(\"Scripting.FileSystemObject\")\n";
		out << "Set shell=CreateObject(\"WScript.Shell\")\n";
		out << "fso.MoveFile \"" << nativePath << "\", \"" << nativeBackupPath << "\"\n";
		out << "fso.MoveFile \"" << nativeOriginPath << "\", \"" << nativePath << "\"\n";
		out << "shell.Run \"" << nativePath << "\"\n";

		QString scriptPath = FS::PathCombine(root(), "update", "update.vbs");

		// we save it
		QFile scriptFile(scriptPath);
		scriptFile.open(QIODevice::WriteOnly);
		scriptFile.write(script.toLocal8Bit().replace("\n", "\r\n"));
		scriptFile.close();

		// we run it
		started = QProcess::startDetached("wscript", {scriptPath}, root());

		// and we quit. conscious thought.
		qApp->quit();
		return;
	}
	started = QProcess::startDetached(finishCmd, args, QDir::currentPath(), &pid);
	// failed to start... ?
	if(!started || pid == -1)
	{
		qWarning() << "Couldn't start new process properly!";
		failedOperationType = Start;
		goto FAILED;
	}
	origin.rmdir(updateFilesDir);
	qApp->quit();
	return;

FAILED:
	qWarning() << "Update failed!";
	bool revertOK = true;
	// if the above failed, roll back changes
	for(auto backup:backups)
	{
		qWarning() << "restoring" << backup.orig << "from" << backup.backup;
		if(!QFile::remove(backup.orig))
		{
			revertOK = false;
			qWarning() << "removing new" << backup.orig << "failed!";
			continue;
		}

		if(!QFile::rename(backup.backup, backup.orig))
		{
			revertOK = false;
			qWarning() << "restoring" << backup.orig << "failed!";
		}
	}
	for(auto backup:trashcan)
	{
		qWarning() << "restoring" << backup.orig << "from" << backup.backup;
		if(!QFile::rename(backup.backup, backup.orig))
		{
			revertOK = false;
			qWarning() << "restoring" << backup.orig << "failed!";
		}
	}
	QString msg;
	if(!revertOK)
	{
		msg = tr("The update failed and then the update revert failed too.\n"
			"You will have to repair MultiMC manually.\n"
				"Please let us know why and how this happened.").arg(failedFile);
	}
	else switch (failedOperationType)
	{
		case Replace:
			msg = tr("Couldn't replace file %1. Changes were reverted.\n"
				"See the MultiMC log file for details.").arg(failedFile);
			break;
		case Delete:
			msg = tr("Couldn't remove file %1. Changes were reverted.\n"
				"See the MultiMC log file for details.").arg(failedFile);
			break;
		case Start:
			msg = tr("The new version didn't start and the update was rolled back.");
			break;
		case Nothing:
		default:
			return;
	}
	QMessageBox::critical(nullptr, tr("Update failed!"), msg);
}

void MultiMC::setIconTheme(const QString& name)
{
	XdgIcon::setThemeName(name);
	IconResourceHandler::setTheme(name);
}

QIcon MultiMC::getThemedIcon(const QString& name)
{
	return XdgIcon::fromTheme(name);
}

void MultiMC::onExit()
{
	if(m_instances)
	{
		m_instances->saveGroupList();
	}
	ENV.destroy();
	if(logFile)
	{
		logFile->flush();
		logFile->close();
	}
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
		return DesktopServices::run(m_settings->get("JsonEditor").toString(), {});
	}
}

#include "MultiMC.moc"
