#pragma once

#include <minecraft/VersionFile.h>
#include <minecraft/Library.h>
#include <QJsonDocument>
#include <ProblemProvider.h>

class MojangVersionFormat
{
friend class OneSixVersionFormat;
protected:
    // does not include libraries
    static void readVersionProperties(const QJsonObject& in, VersionFile* out);
    // does not include libraries
    static void writeVersionProperties(const VersionFile* in, QJsonObject& out);
public:
    // version files / profile patches
    static VersionFilePtr versionFileFromJson(const QJsonDocument &doc, const QString &filename);
    static QJsonDocument versionFileToJson(const VersionFilePtr &patch);

    // libraries
    static LibraryPtr libraryFromJson(ProblemContainer & problems, const QJsonObject &libObj, const QString &filename);
    static QJsonObject libraryToJson(Library *library);
};
