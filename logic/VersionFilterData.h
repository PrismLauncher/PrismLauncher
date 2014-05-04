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

struct VersionFilterData
{
	VersionFilterData();
	// mapping between minecraft versions and FML libraries required
	QMap<QString, QList<FMLlib>> fmlLibsMapping;
	// set of minecraft versions for which using forge installers is blacklisted
	QSet<QString> forgeInstallerBlacklist;
	// set of 'legacy' versions (ones that use the legacy launch)
	QSet<QString> legacyLaunchWhitelist;
};
extern VersionFilterData g_VersionFilterData;
