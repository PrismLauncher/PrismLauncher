#pragma once

#include <minecraft/VersionFile.h>
#include <minecraft/MinecraftProfile.h>
#include <minecraft/Library.h>
#include <QJsonDocument>

class OneSixVersionFormat
{
public:
	// version files / profile patches
	static VersionFilePtr versionFileFromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder);
	static QJsonDocument versionFileToJson(const VersionFilePtr &patch, bool saveOrder);

	// libraries
	static LibraryPtr libraryFromJson(const QJsonObject &libObj, const QString &filename);
	static QJsonObject libraryToJson(Library *library);

	// jar mods
	static JarmodPtr jarModFromJson(const QJsonObject &libObj, const QString &filename, const QString &originalName);
	static QJsonObject jarModtoJson(Jarmod * jarmod);
};
