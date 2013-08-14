// 
//  Copyright 2012 MultiMC Contributors
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
#pragma once

class LegacyInstance;
class BaseInstance;
#include <QList>
#include <QString>
#include <QDir>

#include "Mod.h"

/**
 * A basic mod list.
 * Backed by a folder.
 */
class ModList : public QObject
{
	Q_OBJECT
public:
	ModList(const QString& dir = QString());

	size_t size() { return mods.size(); };
	Mod& operator[](size_t index) { return mods[index]; };
	
	/// Reloads the mod list and returns true if the list changed.
	virtual bool update();

	/// Adds the given mod to the list at the given index.
	virtual bool installMod(const QFileInfo& filename, size_t index = 0);

	/// Deletes the mod at the given index.
	virtual bool deleteMod(size_t index);
	
	/**
	 * move the mod at index to the position N
	 * 0 is the beginning of the list, length() is the end of the list.
	 */
	virtual bool moveMod(size_t from, size_t to) { return false; };
	
	virtual bool isValid();
	
signals:
	virtual void changed();
protected:
	QDir m_dir;
	QList<Mod> mods;
};

/**
 * A jar mod list.
 * Backed by a folder and a file which specifies the load order.
 */
class JarModList : public ModList
{
	Q_OBJECT
public:
	JarModList(const QString& dir, const QString& list_file, LegacyInstance * inst)
		: ModList(dir), m_listfile(list_file), m_inst(inst) {}

	virtual bool update();
	virtual bool installMod(const QString &filename, size_t index);
	virtual bool deleteMod(size_t index);
	virtual bool moveMod(size_t from, size_t to);
protected:
	QString m_listfile;
	LegacyInstance * m_inst;
};
