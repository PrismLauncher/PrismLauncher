#include "PackManifest.h"
#include "Json.h"

static void loadFileV1(Curse::File & f, QJsonObject & file)
{
	f.projectId = Json::requireInteger(file, "projectID");
	f.fileId = Json::requireInteger(file, "fileID");
	f.required = Json::requireBoolean(file, "required");
}

static void loadModloaderV1(Curse::Modloader & m, QJsonObject & modLoader)
{
	m.id = Json::requireString(modLoader, "id");
	m.primary = Json::ensureBoolean(modLoader, QString("primary"), false);
}

static void loadMinecraftV1(Curse::Minecraft & m, QJsonObject & minecraft)
{
	m.version = Json::requireString(minecraft, "version");
	auto arr = Json::ensureArray(minecraft, "modLoaders", QJsonArray());
	for (const auto & item : arr)
	{
		auto obj = Json::requireObject(item);
		Curse::Modloader loader;
		loadModloaderV1(loader, obj);
		m.modLoaders.append(loader);
	}
}

static void loadManifestV1(Curse::Manifest & m, QJsonObject & manifest)
{
	auto mc = Json::requireObject(manifest, "minecraft");
	loadMinecraftV1(m.minecraft, mc);
	m.name = Json::requireString(manifest, "name");
	m.version = Json::requireString(manifest, "version");
	m.author = Json::requireString(manifest, "author");
	auto arr = Json::ensureArray(manifest, "files", QJsonArray());
	for (const auto & item : arr)
	{
		auto obj = Json::requireObject(item);
		Curse::File file;
		loadFileV1(file, obj);
		m.files.append(file);
	}
	m.overrides = Json::ensureString(manifest, "overrides", "overrides");
}

void Curse::loadManifest(Curse::Manifest & m, const QString &filepath)
{
	auto doc = Json::requireDocument(filepath);
	auto obj = Json::requireObject(doc);
	m.manifestType = Json::requireString(obj, "manifestType");
	if(m.manifestType != "minecraftModpack")
	{
		throw JSONValidationError("Not a Curse modpack manifest!");
	}
	m.manifestVersion = Json::requireInteger(obj, "manifestVersion");
	if(m.manifestVersion != 1)
	{
		throw JSONValidationError(QString("Unknown manifest version (%1)").arg(m.manifestVersion));
	}
	loadManifestV1(m, obj);
}
