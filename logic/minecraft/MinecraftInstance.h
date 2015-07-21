#pragma once
#include "BaseInstance.h"
#include "minecraft/Mod.h"
#include <QProcess>

class ModList;

class MinecraftInstance: public BaseInstance
{
public:
	MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
	virtual ~MinecraftInstance() {};

	/// Path to the instance's minecraft directory.
	QString minecraftRoot() const;

	//////  Mod Lists  //////
	virtual std::shared_ptr<ModList> resourcePackList() const
	{
		return nullptr;
	}
	virtual std::shared_ptr<ModList> texturePackList() const
	{
		return nullptr;
	}
	/// get all jar mods applicable to this instance's jar
	virtual QList<Mod> getJarMods() const
	{
		return QList<Mod>();
	}

	//FIXME: nuke?
	virtual std::shared_ptr< BaseVersionList > versionList() const;

	/// get arguments passed to java
	QStringList javaArguments() const;

	/// get variables for launch command variable substitution/environment
	virtual QMap<QString, QString> getVariables() const override;

	/// create an environment for launching processes
	virtual QProcessEnvironment createEnvironment() override;
};

typedef std::shared_ptr<MinecraftInstance> MinecraftInstancePtr;
