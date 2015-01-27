#pragma once
#include "logic/BaseInstance.h"

class MinecraftInstance: public BaseInstance
{
public:
	MinecraftInstance(const QString& rootDir, SettingsObject* settings, QObject* parent = 0);
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
	virtual std::shared_ptr< BaseVersionList > versionList() const;
};

typedef std::shared_ptr<MinecraftInstance> MinecraftInstancePtr;
