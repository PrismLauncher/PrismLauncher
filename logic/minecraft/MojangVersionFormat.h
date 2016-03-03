#pragma once

#include <minecraft/VersionFile.h>
#include <QJsonDocument>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT MojangVersionFormat
{
public:
	// version files / profile patches
	static VersionFilePtr versionFileFromJson(const QJsonDocument &doc, const QString &filename);
	static QJsonDocument profilePatchToJson(const ProfilePatchPtr &patch);
};
