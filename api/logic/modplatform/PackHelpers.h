#pragma once
#include <QList>

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
};

typedef QList<FtbModpack> FtbModpackList;
