/* Copyright 2015-2017 MultiMC Contributors
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

#include <QVector>
#include <QStringList>
#include <QJsonObject>
#include <memory>

#include "minecraft/VersionFile.h"

#include "BaseEntity.h"
#include "Reference.h"

#include "multimc_logic_export.h"

namespace Meta
{
using VersionPtr = std::shared_ptr<class Version>;

class MULTIMC_LOGIC_EXPORT Version : public QObject, public BaseVersion, public BaseEntity
{
	Q_OBJECT
	Q_PROPERTY(QString uid READ uid CONSTANT)
	Q_PROPERTY(QString version READ version CONSTANT)
	Q_PROPERTY(QString type READ type NOTIFY typeChanged)
	Q_PROPERTY(QDateTime time READ time NOTIFY timeChanged)
	Q_PROPERTY(QVector<Reference> requires READ requires NOTIFY requiresChanged)

public: /* con/des */
	explicit Version(const QString &uid, const QString &version);

	QString descriptor() override;
	QString name() override;
	QString typeString() const override;

	QString uid() const { return m_uid; }
	QString version() const { return m_version; }
	QString type() const { return m_type; }
	QDateTime time() const;
	qint64 rawTime() const { return m_time; }
	QVector<Reference> requires() const { return m_requires; }
	VersionFilePtr data() const { return m_data; }

	void merge(const std::shared_ptr<BaseEntity> &other) override;
	void parse(const QJsonObject &obj) override;

	QString localFilename() const override;

public: // for usage by format parsers only
	void setType(const QString &type);
	void setTime(const qint64 time);
	void setRequires(const QVector<Reference> &requires);
	void setData(const VersionFilePtr &data);

signals:
	void typeChanged();
	void timeChanged();
	void requiresChanged();

private:
	QString m_name;
	QString m_uid;
	QString m_version;
	QString m_type;
	qint64 m_time;
	QVector<Reference> m_requires;
	VersionFilePtr m_data;
};
}

Q_DECLARE_METATYPE(Meta::VersionPtr)
