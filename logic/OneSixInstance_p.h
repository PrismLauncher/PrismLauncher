#pragma once

#include "BaseInstance_p.h"
#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include "ModList.h"

struct OneSixInstancePrivate: public BaseInstancePrivate
{
	std::shared_ptr<OneSixVersion> version;
	std::shared_ptr<ModList> loader_mod_list;
	std::shared_ptr<ModList> resource_pack_list;
};