#pragma once

#include <QString>
#include <QVector>

namespace Curse
{
struct File
{
	int projectId = 0;
	int fileId = 0;
	bool required = true;

	// our
	bool resolved = false;
	QString fileName;
	QString url;
};

struct Modloader
{
	QString id;
	bool primary = false;
};

struct Minecraft
{
	QString version;
	QString libraries;
	QVector<Curse::Modloader> modLoaders;
};

struct Manifest
{
	QString manifestType;
	int manifestVersion = 0;
	Curse::Minecraft minecraft;
	QString name;
	QString version;
	QString author;
	QVector<Curse::File> files;
	QString overrides;
};

void loadManifest(Curse::Manifest & m, const QString &filepath);
}
