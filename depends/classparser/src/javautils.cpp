/* Copyright 2013-2014 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "classfile.h"
#include "javautils.h"

#include <QFile>
#include <quazipfile.h>

namespace javautils
{

QString GetMinecraftJarVersion(QString jarName)
{
	QString version = MCVer_Unknown;

	// check if minecraft.jar exists
	QFile jar(jarName);
	if (!jar.exists())
		return version;

	// open minecraft.jar
	QuaZip zip(&jar);
	if (!zip.open(QuaZip::mdUnzip))
		return version;

	// open Minecraft.class
	zip.setCurrentFile("net/minecraft/client/Minecraft.class", QuaZip::csSensitive);
	QuaZipFile Minecraft(&zip);
	if (!Minecraft.open(QuaZipFile::ReadOnly))
		return version;

	// read Minecraft.class
	qint64 size = Minecraft.size();
	char *classfile = new char[size];
	Minecraft.read(classfile, size);

	// parse Minecraft.class
	try
	{
		char *temp = classfile;
		java::classfile MinecraftClass(temp, size);
		java::constant_pool constants = MinecraftClass.constants;
		for (java::constant_pool::container_type::const_iterator iter = constants.begin();
			 iter != constants.end(); iter++)
		{
			const java::constant &constant = *iter;
			if (constant.type != java::constant::j_string_data)
				continue;
			const std::string &str = constant.str_data;
			if (str.compare(0, 20, "Minecraft Minecraft ") == 0)
			{
				version = str.substr(20).data();
				break;
			}
		}
	}
	catch (java::classfile_exception &)
	{
	}

	// clean up
	delete[] classfile;
	Minecraft.close();
	zip.close();
	jar.close();

	return version;
}
}
