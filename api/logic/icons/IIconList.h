#pragma once

#include <QString>

enum IconType : unsigned
{
	Builtin,
	Transient,
	FileBased,
	ICONS_TOTAL,
	ToBeDeleted
};

class IIconList
{
public:
	virtual ~IIconList(){}
	virtual bool addIcon(QString key, QString name, QString path, IconType type) = 0;
};

