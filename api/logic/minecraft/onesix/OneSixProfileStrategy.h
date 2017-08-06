#pragma once
#include "minecraft/ProfileStrategy.h"

class OneSixInstance;

class OneSixProfileStrategy : public ProfileStrategy
{
public:
	OneSixProfileStrategy(OneSixInstance * instance);
	virtual ~OneSixProfileStrategy() {};
	void load() override;
	bool resetOrder() override;
	bool saveOrder(ProfileUtils::PatchOrder order) override;
	bool installJarMods(QStringList filepaths) override;
	bool installCustomJar(QString filepath) override;
	bool removePatch(ProfilePatchPtr patch) override;
	bool customizePatch(ProfilePatchPtr patch) override;
	bool revertPatch(ProfilePatchPtr patch) override;

protected:
	virtual void loadDefaultBuiltinPatches();
	virtual void loadUserPatches();
	void upgradeDeprecatedFiles();

protected:
	OneSixInstance *m_instance;
};
