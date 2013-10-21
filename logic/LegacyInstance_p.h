#pragma once
#include <QString>
#include <settingsobject.h>
#include "BaseInstance_p.h"
#include "ModList.h"
#include <QSharedPointer>

class ModList;

struct LegacyInstancePrivate: public BaseInstancePrivate
{
	std::shared_ptr<ModList> jar_mod_list;
	std::shared_ptr<ModList> core_mod_list;
	std::shared_ptr<ModList> loader_mod_list;
	std::shared_ptr<ModList> texture_pack_list;
};