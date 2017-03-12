#include "FTBPlugin.h"
#include <Env.h>
#include "LegacyFTBInstance.h"
#include "OneSixFTBInstance.h"
#include <BaseInstance.h>
#include <InstanceList.h>
#include <settings/INISettingsObject.h>
#include <FileSystem.h>

#include <QDebug>
#include <QRegularExpression>

#ifdef Q_OS_WIN32
#include <windows.h>
static const int APPDATA_BUFFER_SIZE = 1024;
#endif

static QString getLocalCacheStorageLocation()
{
	QString ftbDefault;
#ifdef Q_OS_WIN32
	wchar_t buf[APPDATA_BUFFER_SIZE];
	if (GetEnvironmentVariableW(L"LOCALAPPDATA", buf, APPDATA_BUFFER_SIZE)) // local
	{
		ftbDefault = QDir(QString::fromWCharArray(buf)).absoluteFilePath("ftblauncher");
	}
	else if (GetEnvironmentVariableW(L"APPDATA", buf, APPDATA_BUFFER_SIZE)) // roaming
	{
		ftbDefault = QDir(QString::fromWCharArray(buf)).absoluteFilePath("ftblauncher");
	}
	else
	{
		qCritical() << "Your LOCALAPPDATA and APPDATA folders are missing!"
			" If you are on windows, this means your system is broken.";
	}
#elif defined(Q_OS_MAC)
	ftbDefault = FS::PathCombine(QDir::homePath(), "Library/Application Support/ftblauncher");
#else
	ftbDefault = QDir::home().absoluteFilePath(".ftblauncher");
#endif
	return ftbDefault;
}


static QString getRoamingStorageLocation()
{
	QString ftbDefault;
#ifdef Q_OS_WIN32
	wchar_t buf[APPDATA_BUFFER_SIZE];
	QString cacheStorage;
	if (GetEnvironmentVariableW(L"APPDATA", buf, APPDATA_BUFFER_SIZE))
	{
		ftbDefault = QDir(QString::fromWCharArray(buf)).absoluteFilePath("ftblauncher");
	}
	else
	{
		qCritical() << "Your APPDATA folder is missing! If you are on windows, this means your system is broken.";
	}
#elif defined(Q_OS_MAC)
	ftbDefault = FS::PathCombine(QDir::homePath(), "Library/Application Support/ftblauncher");
#else
	ftbDefault = QDir::home().absoluteFilePath(".ftblauncher");
#endif
	return ftbDefault;
}

void FTBPlugin::initialize(SettingsObjectPtr globalSettings)
{
	// FTB
	globalSettings->registerSetting("TrackFTBInstances", false);
	QString ftbRoaming = getRoamingStorageLocation();
	QString ftbLocal = getLocalCacheStorageLocation();

	globalSettings->registerSetting("FTBLauncherRoaming", ftbRoaming);
	globalSettings->registerSetting("FTBLauncherLocal", ftbLocal);
	qDebug() << "FTB Launcher paths:" << globalSettings->get("FTBLauncherRoaming").toString()
			 << "and" << globalSettings->get("FTBLauncherLocal").toString();

	globalSettings->registerSetting("FTBRoot");
	if (globalSettings->get("FTBRoot").isNull())
	{
		QString ftbRoot;
		QFile f(QDir(globalSettings->get("FTBLauncherRoaming").toString()).absoluteFilePath("ftblaunch.cfg"));
		qDebug() << "Attempting to read" << f.fileName();
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
				qDebug() << "Failed to get FTB root path";
			}
			else
			{
				qDebug() << "FTB is installed at" << ftbRoot;
				globalSettings->set("FTBRoot", ftbRoot);
			}
		}
		else
		{
			qWarning() << "Couldn't open" << f.fileName() << ":" << f.errorString();
			qWarning() << "This is perfectly normal if you don't have FTB installed";
		}
	}
}
