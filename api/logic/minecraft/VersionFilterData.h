#pragma once
#include <QMap>
#include <QString>
#include <QSet>
#include <QDateTime>

#include "multimc_logic_export.h"

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
	// no new versions below this date will be accepted from Mojang servers
	QDateTime legacyCutoffDate;
	// Libraries that belong to LWJGL
	QSet<QString> lwjglWhitelist;
	// Currently recommended minecraft version
	QString recommendedMinecraftVersion;
};
extern VersionFilterData MULTIMC_LOGIC_EXPORT g_VersionFilterData;
