#pragma once
#include <QList>
#include "qmetatype.h"

//Header for structs etc...
struct FtbModpack {
	QString name;
	QString description;
	QString author;
	QStringList oldVersions;
	QString currentVersion;
	QString mcVersion;
	QString mods;
	QString image;

	//Technical data
	QString dir;
	QString file; //<- Url in the xml, but doesn't make much sense

	bool bugged = true;
	bool broken = true;
};
//We need it for the proxy model
Q_DECLARE_METATYPE(FtbModpack)

typedef QList<FtbModpack> FtbModpackList;
