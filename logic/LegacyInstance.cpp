#include "LegacyInstance.h"
#include "LegacyInstance_p.h"
#include "MinecraftProcess.h"
#include "LegacyUpdate.h"
#include "IconListModel.h"
#include <setting.h>
#include <pathutils.h>
#include <cmdutils.h>
#include "gui/LegacyModEditDialog.h"
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

BaseUpdate* LegacyInstance::doUpdate()
{
	return new LegacyUpdate(this, this);
}

MinecraftProcess* LegacyInstance::prepareForLaunch(QString user, QString session)
{
	MinecraftProcess * proc = new MinecraftProcess(this);
	
	IconList * list = IconList::instance();
	QIcon icon = list->getIcon(iconKey());
	auto pixmap = icon.pixmap(128,128);
	pixmap.save(PathCombine(minecraftRoot(), "icon.png"),"PNG");
	
	// extract the legacy launcher
	QFile(":/launcher/launcher.jar").copy(PathCombine(minecraftRoot(), LAUNCHER_FILE));
	
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
		args << QString("-XX:MaxPermSize=%1m").arg(settings().get("PermGen").toInt());
		args << "-jar" << LAUNCHER_FILE;
		args << user;
		args << session;
		args << windowTitle;
		args << windowSize;
		args << lwjgl;
		proc->setMinecraftArguments(args);
	}
	
	// set the process work path
	proc->setMinecraftWorkdir(minecraftRoot());
	
	return proc;
}

QSharedPointer< ModList > LegacyInstance::coreModList()
{
	I_D(LegacyInstance);
	if(!d->core_mod_list)
	{
		d->core_mod_list.reset(new ModList(coreModsDir()));
	}
	else
		d->core_mod_list->update();
	return d->core_mod_list;
}

QSharedPointer< ModList > LegacyInstance::jarModList()
{
	I_D(LegacyInstance);
	if(!d->jar_mod_list)
	{
		auto list = new ModList(jarModsDir(), modListFile());
		connect(list, SIGNAL(changed()), SLOT(jarModsChanged()));
		d->jar_mod_list.reset(list);
	}
	else
		d->jar_mod_list->update();
	return d->jar_mod_list;
}

void LegacyInstance::jarModsChanged()
{
	setShouldRebuild(true);
}


QSharedPointer< ModList > LegacyInstance::loaderModList()
{
	I_D(LegacyInstance);
	if(!d->loader_mod_list)
	{
		d->loader_mod_list.reset(new ModList(mlModsDir()));
	}
	else
		d->loader_mod_list->update();
	return d->loader_mod_list;
}

QSharedPointer< ModList > LegacyInstance::texturePackList()
{
	I_D(LegacyInstance);
	if(!d->texture_pack_list)
	{
		d->texture_pack_list.reset(new ModList(texturePackDir()));
	}
	else
		d->texture_pack_list->update();
	return d->texture_pack_list;
}


QDialog * LegacyInstance::createModEditDialog ( QWidget* parent )
{
	return new LegacyModEditDialog(this, parent);
}


void LegacyInstance::cleanupAfterRun()
{
	//FIXME: delete the launcher and icons and whatnot.
}


QString LegacyInstance::jarModsDir() const
{
	return PathCombine(instanceRoot(), "instMods");
}

QString LegacyInstance::binDir() const
{
	return PathCombine(minecraftRoot(), "bin");
}

QString LegacyInstance::savesDir() const
{
	return PathCombine(minecraftRoot(), "saves");
}

QString LegacyInstance::mlModsDir() const
{
	return PathCombine(minecraftRoot(), "mods");
}

QString LegacyInstance::coreModsDir() const
{
	return PathCombine(minecraftRoot(), "coremods");
}

QString LegacyInstance::resourceDir() const
{
	return PathCombine(minecraftRoot(), "resources");
}
QString LegacyInstance::texturePackDir() const
{
	return PathCombine(minecraftRoot(), "texturepacks");
}

QString LegacyInstance::runnableJar() const
{
	return PathCombine(binDir(), "minecraft.jar");
}

QString LegacyInstance::modListFile() const
{
	return PathCombine(instanceRoot(), "modlist");
}

QString LegacyInstance::instanceConfigFolder() const
{
	return PathCombine(minecraftRoot(), "config");
}


/*
bool LegacyInstance::shouldUpdateCurrentVersion() const
{
	QFileInfo jar(runnableJar());
	return jar.lastModified().toUTC().toMSecsSinceEpoch() != lastCurrentVersionUpdate();
}

void LegacyInstance::updateCurrentVersion(bool keepCurrent)
{
	QFileInfo jar(runnableJar());
	
	if(!jar.exists())
	{
		setLastCurrentVersionUpdate(0);
		setCurrentVersionId("Unknown");
		return;
	}
	
	qint64 time = jar.lastModified().toUTC().toMSecsSinceEpoch();
	
	setLastCurrentVersionUpdate(time);
	if (!keepCurrent)
	{
		// TODO: Implement GetMinecraftJarVersion function.
		QString newVersion = "Unknown";//javautils::GetMinecraftJarVersion(jar.absoluteFilePath());
		setCurrentVersionId(newVersion);
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
*/
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
QString LegacyInstance::currentVersionId() const
{
	I_D(LegacyInstance);
	return d->m_settings->get ( "JarVersion" ).toString();
}

void LegacyInstance::setCurrentVersionId ( QString val )
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
QString LegacyInstance::intendedVersionId() const
{
	I_D(LegacyInstance);
	return d->m_settings->get ( "IntendedJarVersion" ).toString();
}
bool LegacyInstance::setIntendedVersionId ( QString version )
{
	settings().set("IntendedJarVersion", version);
	setShouldUpdate(true);
	return true;
}
bool LegacyInstance::shouldUpdate() const
{
	I_D(LegacyInstance);
	QVariant var = settings().get ( "ShouldUpdate" );
	if ( !var.isValid() || var.toBool() == false )
	{
		return intendedVersionId() != currentVersionId();
	}
	return true;
}
void LegacyInstance::setShouldUpdate ( bool val )
{
	settings().set ( "ShouldUpdate", val );
}

QString LegacyInstance::defaultBaseJar() const
{
	return "versions/" + intendedVersionId() + "/" + intendedVersionId() + ".jar";
}

QString LegacyInstance::defaultCustomBaseJar() const
{
	return PathCombine(binDir(), "mcbackup.jar");
}

bool LegacyInstance::menuActionEnabled ( QString action_name ) const
{
	return true;
}

QString LegacyInstance::getStatusbarDescription()
{
	if(shouldUpdate())
		return "Legacy : " + currentVersionId() + " -> " + intendedVersionId();
	else
		return "Legacy : " + currentVersionId();
}
