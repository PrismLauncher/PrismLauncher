#pragma once

#include <BaseInstance.h>

#include "multimc_logic_export.h"

// Pseudo-plugin for FTB related things. Super derpy!
class MULTIMC_LOGIC_EXPORT FTBPlugin
{
public:
	static void initialize(SettingsObjectPtr globalSettings);
};
