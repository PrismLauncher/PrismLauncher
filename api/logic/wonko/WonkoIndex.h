/* Copyright 2015 MultiMC Contributors
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

#include <QAbstractListModel>
#include <memory>

#include "BaseWonkoEntity.h"

#include "multimc_logic_export.h"

class Task;
using WonkoVersionListPtr = std::shared_ptr<class WonkoVersionList>;

class MULTIMC_LOGIC_EXPORT WonkoIndex : public QAbstractListModel, public BaseWonkoEntity
{
	Q_OBJECT
public:
	explicit WonkoIndex(QObject *parent = nullptr);
	explicit WonkoIndex(const QVector<WonkoVersionListPtr> &lists, QObject *parent = nullptr);

	enum
	{
		UidRole = Qt::UserRole,
		NameRole,
		ListPtrRole
	};

	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	std::unique_ptr<Task> remoteUpdateTask() override;
	std::unique_ptr<Task> localUpdateTask() override;

	QString localFilename() const override { return "index.json"; }
	QJsonObject serialized() const override;

	// queries
	bool hasUid(const QString &uid) const;
	WonkoVersionListPtr getList(const QString &uid) const;
	WonkoVersionListPtr getListGuaranteed(const QString &uid) const;

	QVector<WonkoVersionListPtr> lists() const { return m_lists; }

public: // for usage by parsers only
	void merge(const BaseWonkoEntity::Ptr &other);

private:
	QVector<WonkoVersionListPtr> m_lists;
	QHash<QString, WonkoVersionListPtr> m_uids;

	void connectVersionList(const int row, const WonkoVersionListPtr &list);
};
