#pragma once
#include <QMap>
#include <QString>
#include <QSet>
#include <QDateTime>

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
	// set of 'legacy' versions that will not show up in the version lists.
	QSet<QString> legacyBlacklist;
	// no new versions below this date will be accepted from Mojang servers
	QDateTime legacyCutoffDate;
	// Libraries that belong to LWJGL
	QSet<QString> lwjglWhitelist;
};
extern VersionFilterData g_VersionFilterData;
