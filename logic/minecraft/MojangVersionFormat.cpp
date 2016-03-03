#include "MojangVersionFormat.h"
#include "onesix/OneSixVersionFormat.h"
#include "MinecraftVersion.h"
#include "VersionBuildError.h"

#include "Json.h"
using namespace Json;
#include "ParseUtils.h"

static const int CURRENT_MINIMUM_LAUNCHER_VERSION = 14;

// FIXME: duplicated in OneSixVersionFormat!
static void readString(const QJsonObject &root, const QString &key, QString &variable)
{
	if (root.contains(key))
	{
		variable = requireString(root.value(key));
	}
}

VersionFilePtr MojangVersionFormat::versionFileFromJson(const QJsonDocument &doc, const QString &filename)
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

	out->name = "Minecraft";
	out->fileId = "net.minecraft";
	out->version = root.value("version").toString();
	out->filename = filename;

	readString(root, "id", out->id);

	readString(root, "mainClass", out->mainClass);
	readString(root, "minecraftArguments", out->overwriteMinecraftArguments);
	readString(root, "type", out->type);

	readString(root, "assets", out->assets);

	out->m_releaseTime = timeFromS3Time(root.value("releaseTime").toString(""));
	out->m_updateTime = timeFromS3Time(root.value("time").toString(""));

	if (root.contains("minimumLauncherVersion"))
	{
		out->minimumLauncherVersion = requireInteger(root.value("minimumLauncherVersion"));
		if (out->minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			out->addProblem(
				PROBLEM_WARNING,
				QObject::tr("The 'minimumLauncherVersion' value of this version (%1) is higher than supported by MultiMC (%2). It might not work properly!")
					.arg(out->minimumLauncherVersion)
					.arg(CURRENT_MINIMUM_LAUNCHER_VERSION));
		}
	}

	if (root.contains("libraries"))
	{
		for (auto libVal : requireArray(root.value("libraries")))
		{
			auto libObj = requireObject(libVal);

			auto lib = OneSixVersionFormat::libraryFromJson(libObj, filename);
			out->addLibs.append(lib);
		}
	}
	return out;
}

static QJsonDocument versionFileToJson(VersionFilePtr patch)
{
	QJsonObject root;
	writeString(root, "id", patch->id);
	writeString(root, "mainClass", patch->mainClass);
	writeString(root, "processArguments", patch->processArguments);
	writeString(root, "minecraftArguments", patch->overwriteMinecraftArguments);
	writeString(root, "type", patch->type);
	writeString(root, "assets", patch->assets);
	writeString(root, "releaseTime", timeToS3Time(patch->m_releaseTime));
	writeString(root, "time", timeToS3Time(patch->m_updateTime));
	if(patch->minimumLauncherVersion != -1)
	{
		root.insert("minimumLauncherVersion", patch->minimumLauncherVersion);
	}

	if (!patch->addLibs.isEmpty())
	{
		QJsonArray array;
		for (auto value: patch->addLibs)
		{
			array.append(OneSixVersionFormat::libraryToJson(value.get()));
		}
		root.insert("libraries", array);
	}
	// write the contents to a json document.
	{
		QJsonDocument out;
		out.setObject(root);
		return out;
	}
}

static QJsonDocument minecraftVersionToJson(MinecraftVersionPtr patch)
{
	if(patch->m_versionSource == Local && patch->getVersionFile())
	{
		return MojangVersionFormat::profilePatchToJson(patch->getVersionFile());
	}
	else
	{
		throw VersionIncomplete(QObject::tr("Can't write incomplete/builtin Minecraft version %1").arg(patch->name()));
	}
}

QJsonDocument MojangVersionFormat::profilePatchToJson(const ProfilePatchPtr &patch)
{
	auto vfile = std::dynamic_pointer_cast<VersionFile>(patch);
	if(vfile)
	{
		return versionFileToJson(vfile);
	}
	auto mversion = std::dynamic_pointer_cast<MinecraftVersion>(patch);
	if(mversion)
	{
		return minecraftVersionToJson(mversion);
	}
	throw VersionIncomplete(QObject::tr("Unhandled object type while processing %1").arg(patch->getPatchName()));
}
