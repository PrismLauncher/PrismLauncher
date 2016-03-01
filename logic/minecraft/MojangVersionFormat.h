#pragma once

#include <minecraft/VersionFile.h>
#include <QJsonDocument>

class MojangVersionFormat
{
public:
	// version files / profile patches
	static VersionFilePtr versionFileFromJson(const QJsonDocument &doc, const QString &filename);
	static QJsonDocument profilePatchToJson(const ProfilePatchPtr &patch);
};
