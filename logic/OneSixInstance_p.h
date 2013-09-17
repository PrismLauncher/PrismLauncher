#pragma once

#include "BaseInstance_p.h"
#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include "ModList.h"

struct OneSixInstancePrivate: public BaseInstancePrivate
{
	QSharedPointer<OneSixVersion> version;
	QSharedPointer<ModList> loader_mod_list;
	QSharedPointer<ModList> resource_pack_list;
};