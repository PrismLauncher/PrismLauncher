#pragma once
#include <logic/minecraft/MinecraftVersion.h>

class FTBVersion : public BaseVersion
{
public:
	FTBVersion(MinecraftVersionPtr parent) : m_version(parent){};

public:
	virtual QString descriptor() override
	{
		return m_version->descriptor();
	}

	virtual QString name() override
	{
		return m_version->name();
	}

	virtual QString typeString() const override
	{
		return m_version->typeString();
	}

	MinecraftVersionPtr getMinecraftVersion()
	{
		return m_version;
	}

private:
	MinecraftVersionPtr m_version;
};
