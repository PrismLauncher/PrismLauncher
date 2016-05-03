#pragma once

#include <QString>
#include "multimc_logic_export.h"

enum IconType : unsigned
{
	Builtin,
	Transient,
	FileBased,
	ICONS_TOTAL,
	ToBeDeleted
};

class MULTIMC_LOGIC_EXPORT IIconList
{
public:
	virtual ~IIconList();
	virtual bool addIcon(QString key, QString name, QString path, IconType type) = 0;
};

