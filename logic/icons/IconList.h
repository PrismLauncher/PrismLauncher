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

#pragma once

#include <QMutex>
#include <QAbstractListModel>
#include <QFile>
#include <QDir>
#include <QtGui/QIcon>
#include <memory>
#include "MMCIcon.h"
#include "logic/settings/Setting.h"

class QFileSystemWatcher;

class IconList : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit IconList(QObject *parent = 0);
	virtual ~IconList() {};

	QIcon getIcon(QString key);
	QIcon getBigIcon(QString key);
	int getIconIndex(QString key);

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

	bool addIcon(QString key, QString name, QString path, MMCIcon::Type type);
	bool deleteIcon(QString key);

	virtual QStringList mimeTypes() const;
	virtual Qt::DropActions supportedDropActions() const;
	virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
							  const QModelIndex &parent);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	void installIcons(QStringList iconFiles);

	void startWatching();
	void stopWatching();

signals:
	void iconUpdated(QString key);

private:
	// hide copy constructor
	IconList(const IconList &) = delete;
	// hide assign op
	IconList &operator=(const IconList &) = delete;
	void reindex();

protected
slots:
	void directoryChanged(const QString &path);
	void fileChanged(const QString &path);
	void SettingChanged(const Setting & setting, QVariant value);
private:
	std::shared_ptr<QFileSystemWatcher> m_watcher;
	bool is_watching;
	QMap<QString, int> name_index;
	QVector<MMCIcon> icons;
	QDir m_dir;
};
