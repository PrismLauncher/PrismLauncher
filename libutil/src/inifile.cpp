/* Copyright 2013 MultiMC Contributors
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

#include "include/inifile.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

INIFile::INIFile()
{
	
}

bool INIFile::saveFile(QString fileName)
{
	// TODO Handle errors.
	QFile file(fileName);
	file.open(QIODevice::WriteOnly);
	QTextStream out(&file);
	
	for (Iterator iter = begin(); iter != end(); iter++)
	{
		out << iter.key() << "=" << iter.value().toString() << "\n";
	}
	
	return true;
}

bool INIFile::loadFile(QString fileName)
{
	// TODO Handle errors.
	QFile file(fileName);
	file.open(QIODevice::ReadOnly);
	QTextStream	in(&file);
	
	QStringList lines = in.readAll().split('\n');
	for (int i = 0; i < lines.count(); i++)
	{
		QString & lineRaw = lines[i];
		// Ignore comments.
		QString line = lineRaw.left(lineRaw.indexOf('#')).trimmed();
		
		int eqPos = line.indexOf('=');
		if(eqPos == -1)
			continue;
		QString key = line.left(eqPos).trimmed();
		QString valueStr = line.right(line.length() - eqPos - 1).trimmed();
		
		QVariant value(valueStr);
		/*
		QString dbg = key;
		dbg += " = ";
		dbg += valueStr;
		qDebug(dbg.toLocal8Bit());
		*/
		this->operator [](key) = value;
	}
	
	return true;
}

QVariant INIFile::get(QString key, QVariant def) const
{
	if (!this->contains(key))
		return def;
	else
		return this->operator [](key);
}

void INIFile::set(QString key, QVariant val)
{
	this->operator [](key) = val;
}
