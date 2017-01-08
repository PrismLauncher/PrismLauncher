/* Copyright 2013-2017 MultiMC Contributors
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
#include "settings/Setting.h"
#include "Env.h" // there is a global icon list inside Env.
#include <icons/IIconList.h>

#include "multimc_gui_export.h"

class QFileSystemWatcher;

class MULTIMC_GUI_EXPORT IconList : public QAbstractListModel, public IIconList
{
	Q_OBJECT
public:
	explicit IconList(const QStringList &builtinPaths, QString path, QObject *parent = 0);
	virtual ~IconList() {};

	QIcon getIcon(const QString &key) const;
	QIcon getBigIcon(const QString &key) const;
	int getIconIndex(const QString &key) const;

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

	bool addThemeIcon(const QString &key);
	bool addIcon(const QString &key, const QString &name, const QString &path, const IconType type) override;
	void saveIcon(const QString &key, const QString &path, const char * format) const override;
	bool deleteIcon(const QString &key) override;
	bool iconFileExists(const QString &key) const override;

	virtual QStringList mimeTypes() const override;
	virtual Qt::DropActions supportedDropActions() const override;
	virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

	void installIcons(const QStringList &iconFiles) override;

	const MMCIcon * icon(const QString &key) const;

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

public slots:
	void directoryChanged(const QString &path);

protected slots:
	void fileChanged(const QString &path);
	void SettingChanged(const Setting & setting, QVariant value);
private:
	std::shared_ptr<QFileSystemWatcher> m_watcher;
	bool is_watching;
	QMap<QString, int> name_index;
	QVector<MMCIcon> icons;
	QDir m_dir;
};
