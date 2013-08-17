#pragma once
#include <QString>
#include <settingsobject.h>
#include "BaseInstance_p.h"
#include "ModList.h"
#include <QSharedPointer>

class ModList;

struct LegacyInstancePrivate: public BaseInstancePrivate
{
	QSharedPointer<ModList> jar_mod_list;
	QSharedPointer<ModList> core_mod_list;
	QSharedPointer<ModList> loader_mod_list;
};