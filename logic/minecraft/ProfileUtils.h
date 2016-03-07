#pragma once
#include "Library.h"
#include "VersionFile.h"

namespace ProfileUtils
{
typedef QStringList PatchOrder;

/// Read and parse a OneSix format order file
bool readOverrideOrders(QString path, PatchOrder &order);

/// Write a OneSix format order file
bool writeOverrideOrders(QString path, const PatchOrder &order);


/// Parse a version file in JSON format
VersionFilePtr parseJsonFile(const QFileInfo &fileInfo, const bool requireOrder);

/// Parse a version file in binary JSON format
VersionFilePtr parseBinaryJsonFile(const QFileInfo &fileInfo);

/// Remove LWJGL from a patch file. This is applied to all Mojang-like profile files.
void removeLwjglFromPatch(VersionFilePtr patch);

}
