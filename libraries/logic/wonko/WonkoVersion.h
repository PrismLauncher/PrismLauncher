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

#include "BaseVersion.h"
#include "BaseWonkoEntity.h"

#include <QVector>
#include <QStringList>
#include <QJsonObject>
#include <memory>

#include "minecraft/VersionFile.h"
#include "WonkoReference.h"

#include "multimc_logic_export.h"

using WonkoVersionPtr = std::shared_ptr<class WonkoVersion>;

class MULTIMC_LOGIC_EXPORT WonkoVersion : public QObject, public BaseVersion, public BaseWonkoEntity
{
	Q_OBJECT
	Q_PROPERTY(QString uid READ uid CONSTANT)
	Q_PROPERTY(QString version READ version CONSTANT)
	Q_PROPERTY(QString type READ type NOTIFY typeChanged)
	Q_PROPERTY(QDateTime time READ time NOTIFY timeChanged)
	Q_PROPERTY(QVector<WonkoReference> requires READ requires NOTIFY requiresChanged)
public:
	explicit WonkoVersion(const QString &uid, const QString &version);

	QString descriptor() override;
	QString name() override;
	QString typeString() const override;

	QString uid() const { return m_uid; }
	QString version() const { return m_version; }
	QString type() const { return m_type; }
	QDateTime time() const;
	qint64 rawTime() const { return m_time; }
	QVector<WonkoReference> requires() const { return m_requires; }
	VersionFilePtr data() const { return m_data; }

	std::unique_ptr<Task> remoteUpdateTask() override;
	std::unique_ptr<Task> localUpdateTask() override;
	void merge(const std::shared_ptr<BaseWonkoEntity> &other) override;

	QString localFilename() const override;
	QJsonObject serialized() const override;

public: // for usage by format parsers only
	void setType(const QString &type);
	void setTime(const qint64 time);
	void setRequires(const QVector<WonkoReference> &requires);
	void setData(const VersionFilePtr &data);

signals:
	void typeChanged();
	void timeChanged();
	void requiresChanged();

private:
	QString m_uid;
	QString m_version;
	QString m_type;
	qint64 m_time;
	QVector<WonkoReference> m_requires;
	VersionFilePtr m_data;
};

Q_DECLARE_METATYPE(WonkoVersionPtr)
