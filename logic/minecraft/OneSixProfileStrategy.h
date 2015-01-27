#pragma once
#include "ProfileStrategy.h"

class OneSixInstance;

class OneSixProfileStrategy : public ProfileStrategy
{
public:
	OneSixProfileStrategy(OneSixInstance * instance);
	virtual ~OneSixProfileStrategy() {};
	virtual void load() override;
	virtual bool resetOrder() override;
	virtual bool saveOrder(ProfileUtils::PatchOrder order) override;
	virtual bool installJarMods(QStringList filepaths) override;
	virtual bool removePatch(ProfilePatchPtr patch) override;

protected:
	void loadDefaultBuiltinPatches();
	void loadUserPatches();
	void upgradeDeprecatedFiles();

protected:
	OneSixInstance *m_instance;
};