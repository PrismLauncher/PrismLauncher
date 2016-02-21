#pragma once
#include "../minecraft/ProfileStrategy.h"
#include "../minecraft/OneSixProfileStrategy.h"

class OneSixFTBInstance;

class FTBProfileStrategy : public OneSixProfileStrategy
{
public:
	FTBProfileStrategy(OneSixFTBInstance * instance);
	virtual ~FTBProfileStrategy() {};
	virtual void load() override;
	virtual bool resetOrder() override;
	virtual bool saveOrder(ProfileUtils::PatchOrder order) override;
	virtual bool installJarMods(QStringList filepaths) override;
	virtual bool customizePatch (ProfilePatchPtr patch) override;
	virtual bool revertPatch (ProfilePatchPtr patch) override;

protected:
	virtual void loadDefaultBuiltinPatches() override;
};
