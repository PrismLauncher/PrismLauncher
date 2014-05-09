#pragma once

#include <memory>
#include <QList>
#include "JarMod.h"

class VersionFinal;
class VersionPatch
{
public:
	virtual ~VersionPatch(){};
	virtual void applyTo(VersionFinal *version) = 0;
	
	virtual bool isVanilla() = 0;
	virtual bool hasJarMods() = 0;
	virtual QList<JarmodPtr> getJarMods() = 0;
	
	virtual bool isMoveable()
	{
		return getOrder() >= 0;
	}
	virtual void setOrder(int order) = 0;
	virtual int getOrder() = 0;
	
	virtual QString getPatchID() = 0;
	virtual QString getPatchName() = 0;
	virtual QString getPatchVersion() = 0;
	virtual QString getPatchFilename() = 0;
};

typedef std::shared_ptr<VersionPatch> VersionPatchPtr;
