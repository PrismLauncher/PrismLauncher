#pragma once

#include <minecraft/VersionFile.h>
#include <minecraft/MinecraftProfile.h>
#include <QJsonDocument>

namespace OneSixVersionFormat {
	std::shared_ptr<MinecraftProfile> readProfileFromSingleFile(const QJsonObject &obj);
	VersionFilePtr fromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder);
	QJsonDocument toJson(const ProfilePatchPtr &patch, bool saveOrder);
}
