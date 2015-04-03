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
	virtual bool removePatch(ProfilePatchPtr patch) override;

protected:
	void loadDefaultBuiltinPatches();
	void loadUserPatches();
};
