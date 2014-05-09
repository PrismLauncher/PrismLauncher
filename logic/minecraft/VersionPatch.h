#pragma once

#include <memory>

class VersionFinal;
class VersionPatch
{
public:
	virtual ~VersionPatch(){};
	virtual void applyTo(VersionFinal *version) = 0;
	virtual bool isVanilla() = 0;
	virtual bool hasJarMods() = 0;
};

typedef std::shared_ptr<VersionPatch> VersionPatchPtr;
