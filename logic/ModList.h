// 
//  Copyright 2013 MultiMC Contributors
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
#include <QAbstractListModel>
#include "Mod.h"

class QFileSystemWatcher;

/**
 * A legacy mod list.
 * Backed by a folder.
 */
class ModList : public QAbstractListModel
{
	Q_OBJECT
public:
	ModList(const QString& dir, const QString& list_file = QString());

	virtual QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const;
	virtual int rowCount ( const QModelIndex& parent = QModelIndex() ) const
	{
		return size();
	};
	virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
	virtual int columnCount ( const QModelIndex& parent ) const;
	
	size_t size() const { return mods.size(); };
	bool empty() const { return size() == 0; }
	Mod& operator[](size_t index) { return mods[index]; };
	
	/// Reloads the mod list and returns true if the list changed.
	virtual bool update();

	/**
	 * Adds the given mod to the list at the given index - if the list supports custom ordering
	 */
	virtual bool installMod(const QFileInfo& filename, int index = 0);

	/// Deletes the mod at the given index.
	virtual bool deleteMod(int index);
	
	/// Deletes all the selected mods
	virtual bool deleteMods( int first, int last );
	
	/**
	 * move the mod at index to the position N
	 * 0 is the beginning of the list, length() is the end of the list.
	 */
	virtual bool moveModTo(int from, int to);
	
	/**
	 * move the mod at index one position upwards
	 */
	virtual bool moveModUp(int from);
	virtual bool moveModsUp( int first, int last );
	
	/**
	 * move the mod at index one position downwards
	 */
	virtual bool moveModDown(int from);
	virtual bool moveModsDown( int first, int last );
	
	/// flags, mostly to support drag&drop
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	/// get data for drag action
	virtual QMimeData* mimeData(const QModelIndexList& indexes) const;
	/// get the supported mime types
	virtual QStringList mimeTypes() const;
	/// process data from drop action
	virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
	/// what drag actions do we support?
	virtual Qt::DropActions supportedDragActions() const;
	
	/// what drop actions do we support?
	virtual Qt::DropActions supportedDropActions() const;
	
	void startWatching();
	void stopWatching();
	
	virtual bool isValid();
	
	QDir dir()
	{
		return m_dir;
	}
private:
	QStringList readListFile();
	bool saveListFile();
private slots:
	void directoryChanged(QString path);
	
signals:
	void changed();
protected:
	QFileSystemWatcher * m_watcher;
	bool is_watching;
	QDir m_dir;
	QString m_list_file;
	QString m_list_id;
	QList<Mod> mods;
};
