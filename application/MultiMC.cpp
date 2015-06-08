#include "MultiMC.h"
#include "BuildConfig.h"

#include <iostream>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QStringList>
#include <QDesktopServices>
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

#include "pathutils.h"
#include "cmdutils.h"
#include <xdgicon.h>
#include "settings/INISettingsObject.h"
#include "settings/Setting.h"

#include "trans/TranslationDownloader.h"
#include "resources/Resource.h"
#include "resources/IconResourceHandler.h"

#include "ftb/FTBPlugin.h"

using namespace Util::Commandline;

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
			std::cout << "Version " << BuildConfig.VERSION_STR.toStdString() << std::endl;
			std::cout << "Git " << BuildConfig.GIT_COMMIT.toStdString() << std::endl;
			m_status = MultiMC::Succeeded;
			return;
		}
	}

	QString origcwdPath = QDir::currentPath();
	QString binPath = applicationDirPath();
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

	if (!ensureFolderPathExists(dataPath) || !QDir::setCurrent(dataPath))
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
		QDir foo(PathCombine(binPath, ".."));
		rootPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
		rootPath = binPath;
#elif defined(Q_OS_MAC)
		QDir foo(PathCombine(binPath, "../.."));
		rootPath = foo.absolutePath();
#endif
	}

// static data paths... mostly just for translations
#ifdef Q_OS_LINUX
	QDir foo(PathCombine(binPath, ".."));
	staticDataPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
	staticDataPath = binPath;
#elif defined(Q_OS_MAC)
	QDir foo(PathCombine(rootPath, "Contents/Resources"));
	staticDataPath = foo.absolutePath();
#endif

	// init the logger
	initLogger();

	qDebug() << "MultiMC 5, (c) 2013-2015 MultiMC Contributors";
	qDebug() << "Version                    : " << BuildConfig.VERSION_STR;
	qDebug() << "Git commit                 : " << BuildConfig.GIT_COMMIT;
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
	qDebug() << "Static data path           : " << staticDataPath;

	// load settings
	initGlobalSettings(test_mode);

	// load translations
	initTranslations();

	// initialize the updater
	m_updateChecker.reset(new UpdateChecker(BuildConfig.CHANLIST_URL, BuildConfig.VERSION_CHANNEL, BuildConfig.VERSION_BUILD));

	m_translationChecker.reset(new TranslationDownloader());

	// load icons
	initIcons();

	// and instances
	auto InstDirSetting = m_settings->getSetting("InstanceDir");
	// instance path: check for problems with '!' in instance path and warn the user in the log
	// and rememer that we have to show him a dialog when the gui starts (if it does so)
	QString instDir = m_settings->get("InstanceDir").toString();
	qDebug() << "Instance path              : " << instDir;
	if (checkProblemticPathJava(QDir(instDir)))
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
	ENV.initHttpMetaCache(rootPath, staticDataPath);

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

void MultiMC::initSSL()
{
#ifdef Q_OS_MAC
	Q_INIT_RESOURCE(certs);
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
	if (m_mmc_translator->load("mmc_" + locale.bcp47Name(), staticDataPath + "/translations"))
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

	Resource::registerTransformer([](const QVariantMap &map) -> QIcon
	{
		QIcon icon;
		for (auto it = map.constBegin(); it != map.constEnd(); ++it)
		{
			icon.addFile(it.key(), QSize(it.value().toInt(), it.value().toInt()));
		}
		return icon;
	});
	Resource::registerTransformer([](const QVariantMap &map) -> QPixmap
	{
		QVariantList sizes = map.values();
		if (sizes.isEmpty())
		{
			return QPixmap();
		}
		std::sort(sizes.begin(), sizes.end());
		if (sizes.last().toInt() != -1) // only scalable available
		{
			return QPixmap(map.key(sizes.last()));
		}
		else
		{
			return QPixmap();
		}
	});
	Resource::registerTransformer([](const QByteArray &data) -> QPixmap
	{ return QPixmap::fromImage(QImage::fromData(data)); });
	Resource::registerTransformer([](const QByteArray &data) -> QIcon
	{ return QIcon(QPixmap::fromImage(QImage::fromData(data))); });
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

std::shared_ptr<JavaVersionList> MultiMC::javalist()
{
	if (!m_javalist)
	{
		m_javalist.reset(new JavaVersionList());
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
#ifdef WINDOWS
	QString finishCmd = applicationFilePath();
#elif LINUX
	QString finishCmd = PathCombine(root(), "MultiMC");
#elif OSX
	QString finishCmd = applicationFilePath();
#else
#error Unsupported operating system.
#endif

	QString backupPath = PathCombine(root(), "update-backup");
	QString trashPath = PathCombine(root(), "update-trash");
	if(!ensureFolderPathExists(backupPath))
	{
		qWarning() << "couldn't create folder" << backupPath;
		return;
	}
	if(!ensureFolderPathExists(trashPath))
	{
		qWarning() << "couldn't create folder" << trashPath;
		return;
	}
	struct BackupEntry
	{
		QString orig;
		QString backup;
	};
	QList <BackupEntry> backups;
	QList <BackupEntry> trashcan;
	for(auto op: operations)
	{
		switch(op.type)
		{
			case GoUpdate::Operation::OP_COPY:
			{
				QFileInfo replaced (PathCombine(root(), op.dest));
				if(replaced.exists())
				{
					QString backupFilePath = PathCombine(backupPath, replaced.completeBaseName());
					if(!QFile::rename(replaced.absoluteFilePath(), backupFilePath))
					{
						qWarning() << "Couldn't rename:" << replaced.absoluteFilePath() << "to" << backupFilePath;
						goto FAILED;
					}
					BackupEntry be;
					be.orig = replaced.absoluteFilePath();
					be.backup = backupFilePath;
					backups.append(be);
				}
				if(!QFile::copy(op.file, replaced.absoluteFilePath()))
				{
					qWarning() << "Couldn't copy:" << op.file << "to" << replaced.absoluteFilePath();
					goto FAILED;
				}
				QFile::setPermissions(replaced.absoluteFilePath(), unixModeToPermissions(op.mode));
			}
			break;
			case GoUpdate::Operation::OP_DELETE:
			{
				QString trashFilePath = PathCombine(backupPath, op.file);
				QString origFilePath = PathCombine(root(), op.file);
				if(QFile::exists(origFilePath))
				{
					QFile::rename(origFilePath, trashFilePath);
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
	started = QProcess::startDetached(finishCmd, args, QDir::currentPath(), &pid);
	// failed to start... ?
	if(!started || pid == -1)
	{
		qWarning() << "Couldn't start new process properly!";
		goto FAILED;
	}
	// now clean up the backed up stuff.
	for(auto backup:backups)
	{
		QFile::remove(backup.backup);
	}
	for(auto backup:trashcan)
	{
		QFile::remove(backup.backup);
	}
	qApp->quit();
	return;

FAILED:
	qWarning() << "Update failed!";
	// if the above failed, roll back changes
	for(auto backup:backups)
	{
		qWarning() << "restoring" << backup.orig << "from" << backup.backup;
		QFile::remove(backup.orig);
		QFile::rename(backup.backup, backup.orig);
	}
	for(auto backup:trashcan)
	{
		qWarning() << "restoring" << backup.orig << "from" << backup.backup;
		QFile::rename(backup.backup, backup.orig);
	}
	// and do nothing
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
		return QDesktopServices::openUrl(QUrl::fromLocalFile(file));
	}
	else
	{
		return QProcess::startDetached(m_settings->get("JsonEditor").toString(), QStringList()
																					 << file);
	}
}

#include "MultiMC.moc"
