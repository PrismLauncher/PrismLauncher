#pragma once

#include <ProblemProvider.h>
#include <minecraft/Library.h>
#include <minecraft/VersionFile.h>
#include <QJsonDocument>

#include "Result.h"

class MojangVersionFormat {
    friend class OneSixVersionFormat;

   protected:
    // does not include libraries
    static Result<> readVersionProperties(const QJsonObject& in, VersionFile* out);
    // does not include libraries
    static void writeVersionProperties(const VersionFile* in, QJsonObject& out);

   public:
    // version files / profile patches
    static Result<VersionFilePtr> versionFileFromJson(const QJsonDocument& doc, const QString& filename);
    static QJsonDocument versionFileToJson(const VersionFilePtr& patch);

    // libraries
    static Result<LibraryPtr> libraryFromJson(ProblemContainer& problems, const QJsonObject& libObj, const QString& filename);
    static QJsonObject libraryToJson(Library* library);
};
