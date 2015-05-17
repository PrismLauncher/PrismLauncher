#pragma once

#include <memory>
#include <QList>
#include <QJsonDocument>
#include "JarMod.h"

class MinecraftProfile;
class ProfilePatch
{
public:
	virtual ~ProfilePatch(){};
	virtual void applyTo(MinecraftProfile *version) = 0;
	virtual QJsonDocument toJson(bool saveOrder) = 0;

	virtual bool isMinecraftVersion() = 0;
	virtual bool hasJarMods() = 0;
	virtual QList<JarmodPtr> getJarMods() = 0;

	virtual bool isMoveable() = 0;
	virtual bool isCustomizable() = 0;
	virtual bool isRevertible() = 0;
	virtual bool isRemovable() = 0;
	virtual bool isCustom() = 0;
	virtual bool isEditable() = 0;
	virtual bool isVersionChangeable() = 0;

	virtual void setOrder(int order) = 0;
	virtual int getOrder() = 0;

	virtual QString getPatchID() = 0;
	virtual QString getPatchName() = 0;
	virtual QString getPatchVersion() = 0;
	virtual QString getPatchFilename() = 0;
};

typedef std::shared_ptr<ProfilePatch> ProfilePatchPtr;
