#pragma once
#include "minecraft/ProfileStrategy.h"
#include "minecraft/onesix/OneSixProfileStrategy.h"

class OneSixFTBInstance;

class FTBProfileStrategy : public OneSixProfileStrategy
{
public:
	FTBProfileStrategy(OneSixFTBInstance * instance);
	virtual ~FTBProfileStrategy() {};
	void load() override;
	bool resetOrder() override;
	bool saveOrder(ProfileUtils::PatchOrder order) override;
	bool installJarMods(QStringList filepaths) override;
	bool installCustomJar(QString filepath) override;
	bool customizePatch (ProfilePatchPtr patch) override;
	bool revertPatch (ProfilePatchPtr patch) override;

protected:
	void loadDefaultBuiltinPatches() override;
};
