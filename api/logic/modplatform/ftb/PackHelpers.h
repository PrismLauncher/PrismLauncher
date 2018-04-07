#pragma once
#include <QList>
#include <QString>
#include "qmetatype.h"

//Header for structs etc...
enum FtbPackType {
	Public,
	ThirdParty,
	Private
};

struct FtbModpack {
	QString name;
	QString description;
	QString author;
	QStringList oldVersions;
	QString currentVersion;
	QString mcVersion;
	QString mods;
	QString logo;

	//Technical data
	QString dir;
	QString file; //<- Url in the xml, but doesn't make much sense

	bool bugged = false;
	bool broken = false;

	FtbPackType type;
};

//We need it for the proxy model
Q_DECLARE_METATYPE(FtbModpack)

typedef QList<FtbModpack> FtbModpackList;
