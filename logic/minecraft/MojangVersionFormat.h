#pragma once

#include <minecraft/VersionFile.h>
#include <QJsonDocument>

namespace MojangVersionFormat {
	VersionFilePtr fromJson(const QJsonDocument &doc, const QString &filename);
}
