#pragma once
#include <QMap>
#include <QString>
#include <QSet>

struct FMLlib
{
	QString filename;
	QString checksum;
	bool ours;
};

struct ForgeData
{
	ForgeData();
	// mapping between minecraft versions and FML libraries required
	QMap<QString, QList<FMLlib>> fmlLibsMapping;
	// set of minecraft versions for which using forge installers is blacklisted
	QSet<QString> forgeInstallerBlacklist;
};
extern ForgeData g_forgeData;
