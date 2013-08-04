#include "LegacyInstance.h"
#include "LegacyInstance_p.h"
#include "MinecraftProcess.h"
#include <setting.h>
#include <pathutils.h>
#include <cmdutils.h>
#include <QFileInfo>
#include <QDir>
#include <QImage>

#define LAUNCHER_FILE "MultiMCLauncher.jar"

LegacyInstance::LegacyInstance(const QString& rootDir, SettingsObject* settings, QObject* parent)
	:BaseInstance( new LegacyInstancePrivate(),rootDir, settings, parent)
{
	settings->registerSetting(new Setting("NeedsRebuild", true));
	settings->registerSetting(new Setting("ShouldUpdate", false));
	settings->registerSetting(new Setting("JarVersion", "Unknown"));
	settings->registerSetting(new Setting("LwjglVersion", "2.9.0"));
	settings->registerSetting(new Setting("IntendedJarVersion", ""));
}

QString LegacyInstance::minecraftDir() const
{
	QFileInfo mcDir(PathCombine(rootDir(), "minecraft"));
	QFileInfo dotMCDir(PathCombine(rootDir(), ".minecraft"));
	
	if (dotMCDir.exists() && !mcDir.exists())
        return dotMCDir.filePath();
	else
		return mcDir.filePath();
}

OneSixUpdate* LegacyInstance::doUpdate()
{
	// legacy instances no longer update
	return nullptr;
}

MinecraftProcess* LegacyInstance::prepareForLaunch(QString user, QString session)
{
	MinecraftProcess * proc = new MinecraftProcess(this);
	
	// FIXME: extract the icon
	// QImage(":/icons/instances/" + iconKey()).save(PathCombine(minecraftDir(), "icon.png"));
	
	// extract the legacy launcher
	QFile(":/launcher/launcher.jar").copy(PathCombine(minecraftDir(), LAUNCHER_FILE));
	
	// set the process arguments
	{
		QStringList args;
		
		// window size
		QString windowSize;
		if (settings().get("LaunchMaximized").toBool())
			windowSize = "max";
		else
			windowSize = QString("%1x%2").
					arg(settings().get("MinecraftWinWidth").toInt()).
					arg(settings().get("MinecraftWinHeight").toInt());
		
		// window title
		QString windowTitle;
		windowTitle.append("MultiMC: ").append(name());
		
		// Java arguments
		args.append(Util::Commandline::splitArgs(settings().get("JvmArgs").toString()));
		
#ifdef OSX
		// OSX dock icon and name
		args << "-Xdock:icon=icon.png";
		args << QString("-Xdock:name=\"%1\"").arg(windowTitle);
#endif
		
		QString lwjgl = QDir(globalSettings->get("LWJGLDir").toString() + "/" + lwjglVersion()).absolutePath();
		
		// launcher arguments
		args << QString("-Xms%1m").arg(settings().get("MinMemAlloc").toInt());
		args << QString("-Xmx%1m").arg(settings().get("MaxMemAlloc").toInt());
		args << "-jar" << LAUNCHER_FILE;
		args << user;
		args << session;
		args << windowTitle;
		args << windowSize;
		args << lwjgl;
		proc->setMinecraftArguments(args);
	}
	
	// set the process work path
	proc->setMinecraftWorkdir(minecraftDir());
	
	return proc;
}


QString LegacyInstance::instModsDir() const
{
	return PathCombine(rootDir(), "instMods");
}

QString LegacyInstance::binDir() const
{
	return PathCombine(minecraftDir(), "bin");
}

QString LegacyInstance::savesDir() const
{
	return PathCombine(minecraftDir(), "saves");
}

QString LegacyInstance::mlModsDir() const
{
	return PathCombine(minecraftDir(), "mods");
}

QString LegacyInstance::coreModsDir() const
{
	return PathCombine(minecraftDir(), "coremods");
}

QString LegacyInstance::resourceDir() const
{
	return PathCombine(minecraftDir(), "resources");
}

QString LegacyInstance::mcJar() const
{
	return PathCombine(binDir(), "minecraft.jar");
}

QString LegacyInstance::mcBackup() const
{
	return PathCombine(binDir(), "mcbackup.jar");
}

QString LegacyInstance::modListFile() const
{
	return PathCombine(rootDir(), "modlist");
}

bool LegacyInstance::shouldUpdateCurrentVersion() const
{
	QFileInfo jar(mcJar());
	return jar.lastModified().toUTC().toMSecsSinceEpoch() != lastCurrentVersionUpdate();
}

void LegacyInstance::updateCurrentVersion(bool keepCurrent)
{
	QFileInfo jar(mcJar());
	
	if(!jar.exists())
	{
		setLastCurrentVersionUpdate(0);
		setCurrentVersion("Unknown");
		return;
	}
	
	qint64 time = jar.lastModified().toUTC().toMSecsSinceEpoch();
	
	setLastCurrentVersionUpdate(time);
	if (!keepCurrent)
	{
		// TODO: Implement GetMinecraftJarVersion function.
		QString newVersion = "Unknown";//javautils::GetMinecraftJarVersion(jar.absoluteFilePath());
		setCurrentVersion(newVersion);
	}
}
qint64 LegacyInstance::lastCurrentVersionUpdate() const
{
	I_D(LegacyInstance);
	return d->m_settings->get ( "lastVersionUpdate" ).value<qint64>();
}
void LegacyInstance::setLastCurrentVersionUpdate ( qint64 val )
{
	I_D(LegacyInstance);
	d->m_settings->set ( "lastVersionUpdate", val );
}
bool LegacyInstance::shouldRebuild() const
{
	I_D(LegacyInstance);
	return d->m_settings->get ( "NeedsRebuild" ).toBool();
}
void LegacyInstance::setShouldRebuild ( bool val )
{
	I_D(LegacyInstance);
	d->m_settings->set ( "NeedsRebuild", val );
}
QString LegacyInstance::currentVersion() const
{
	I_D(LegacyInstance);
	return d->m_settings->get ( "JarVersion" ).toString();
}
void LegacyInstance::setCurrentVersion ( QString val )
{
	I_D(LegacyInstance);
	d->m_settings->set ( "JarVersion", val );
}
QString LegacyInstance::lwjglVersion() const
{
	I_D(LegacyInstance);
	return d->m_settings->get ( "LwjglVersion" ).toString();
}
void LegacyInstance::setLWJGLVersion ( QString val )
{
	I_D(LegacyInstance);
	d->m_settings->set ( "LwjglVersion", val );
}
QString LegacyInstance::intendedVersionId()
{
	I_D(LegacyInstance);
	return d->m_settings->get ( "IntendedJarVersion" ).toString();
}
bool LegacyInstance::setIntendedVersionId ( QString version )
{
	/*
	I_D(LegacyInstance);
	d->m_settings->set ( "IntendedJarVersion", val );
	*/
	return false;
}
bool LegacyInstance::shouldUpdate() const
{
	/*
	I_D(LegacyInstance);
	QVariant var = d->m_settings->get ( "ShouldUpdate" );
	if ( !var.isValid() || var.toBool() == false )
	{
		return intendedVersionId() != currentVersion();
	}
	return true;
	*/
	return false;
}
void LegacyInstance::setShouldUpdate ( bool val )
{
	/*
	I_D(LegacyInstance);
	d->m_settings->set ( "ShouldUpdate", val );
	*/
}
