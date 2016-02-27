#include "MojangVersionFormat.h"

#include "Json.h"
using namespace Json;

static const int CURRENT_MINIMUM_LAUNCHER_VERSION = 14;

// FIXME: duplicated in OneSixVersionFormat!
static void readString(const QJsonObject &root, const QString &key, QString &variable)
{
	if (root.contains(key))
	{
		variable = requireString(root.value(key));
	}
}

VersionFilePtr MojangVersionFormat::fromJson(const QJsonDocument &doc, const QString &filename)
{
	VersionFilePtr out(new VersionFile());
	if (doc.isEmpty() || doc.isNull())
	{
		throw JSONValidationError(filename + " is empty or null");
	}
	if (!doc.isObject())
	{
		throw JSONValidationError(filename + " is not an object");
	}

	QJsonObject root = doc.object();

	out->name = root.value("name").toString();
	out->fileId = root.value("fileId").toString();
	out->version = root.value("version").toString();
	out->mcVersion = root.value("mcVersion").toString();
	out->filename = filename;

	readString(root, "id", out->id);

	readString(root, "mainClass", out->mainClass);
	readString(root, "appletClass", out->appletClass);
	readString(root, "minecraftArguments", out->overwriteMinecraftArguments);
	readString(root, "type", out->type);

	readString(root, "assets", out->assets);

	if (root.contains("minimumLauncherVersion"))
	{
		auto minimumLauncherVersion = requireInteger(root.value("minimumLauncherVersion"));
		if (minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			out->addProblem(
				PROBLEM_WARNING,
				QObject::tr("The 'minimumLauncherVersion' value of this version (%1) is higher than supported by MultiMC (%2). It might not work properly!")
					.arg(minimumLauncherVersion)
					.arg(CURRENT_MINIMUM_LAUNCHER_VERSION));
		}
	}

	if (root.contains("libraries"))
	{
		out->shouldOverwriteLibs = true;
		for (auto libVal : requireArray(root.value("libraries")))
		{
			auto libObj = requireObject(libVal);

			auto lib = RawLibrary::fromJson(libObj, filename);
			out->overwriteLibs.append(lib);
		}
	}
	return out;
}
