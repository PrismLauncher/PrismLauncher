/* Copyright 2013-2015 MultiMC Contributors
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

#include "settings/INIFile.h"
#include <FileSystem.h>

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QSaveFile>
#include <QDebug>

INIFile::INIFile()
{
}

QString INIFile::unescape(QString orig)
{
	QString out;
	QChar prev = 0;
	for(auto c: orig)
	{
		if(prev == '\\')
		{
			if(c == 'n')
				out += '\n';
			else if (c == 't')
				out += '\t';
			else
				out += c;
			prev = 0;
		}
		else
		{
			if(c == '\\')
			{
				prev = c;
				continue;
			}
			out += c;
			prev = 0;
		}
	}
	return out;
}

QString INIFile::escape(QString orig)
{
	QString out;
	for(auto c: orig)
	{
		if(c == '\n')
			out += "\\n";
		else if (c == '\t')
			out += "\\t";
		else if(c == '\\')
			out += "\\\\";
		else
			out += c;
	}
	return out;
}

bool INIFile::saveFile(QString fileName)
{
	QByteArray outArray;
	for (Iterator iter = begin(); iter != end(); iter++)
	{
		QString value = iter.value().toString();
		value = escape(value);
		outArray.append(iter.key().toUtf8());
		outArray.append('=');
		outArray.append(value.toUtf8());
		outArray.append('\n');
	}

	try
	{
		FS::write(fileName, outArray);
	}
	catch (Exception & e)
	{
		qCritical() << e.what();
		return false;
	}

	return true;
}


bool INIFile::loadFile(QString fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	bool success = loadFile(file.readAll());
	file.close();
	return success;
}

bool INIFile::loadFile(QByteArray file)
{
	QTextStream in(file);
	in.setCodec("UTF-8");

	QStringList lines = in.readAll().split('\n');
	for (int i = 0; i < lines.count(); i++)
	{
		QString &lineRaw = lines[i];
		// Ignore comments.
		QString line = lineRaw.left(lineRaw.indexOf('#')).trimmed();

		int eqPos = line.indexOf('=');
		if (eqPos == -1)
			continue;
		QString key = line.left(eqPos).trimmed();
		QString valueStr = line.right(line.length() - eqPos - 1).trimmed();

		valueStr = unescape(valueStr);

		QVariant value(valueStr);
		this->operator[](key) = value;
	}

	return true;
}

QVariant INIFile::get(QString key, QVariant def) const
{
	if (!this->contains(key))
		return def;
	else
		return this->operator[](key);
}

void INIFile::set(QString key, QVariant val)
{
	this->operator[](key) = val;
}
