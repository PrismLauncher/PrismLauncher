#pragma once
#include <QString>
namespace javautils
{
	/*
	 * Get the version from a minecraft.jar by parsing its class files. Expensive!
	 */
	QString GetMinecraftJarVersion(QString jar);
}