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

#pragma once

#include <QObject>
#include <QDateTime>

#include <settingsobject.h>

#include "inifile.h"
#include "lists/BaseVersionList.h"
#include "logic/auth/MojangAccount.h"

class QDialog;
class Task;
class MinecraftProcess;
class OneSixUpdate;
class InstanceList;
class BaseInstancePrivate;

/*!
 * \brief Base class for instances.
 * This class implements many functions that are common between instances and
 * provides a standard interface for all instances.
 *
 * To create a new instance type, create a new class inheriting from this class
 * and implement the pure virtual functions.
 */
class BaseInstance : public QObject
{
	Q_OBJECT
protected:
	/// no-touchy!
	BaseInstance(BaseInstancePrivate *d, const QString &rootDir, SettingsObject *settings,
				 QObject *parent = 0);

public:
	/// virtual destructor to make sure the destruction is COMPLETE
	virtual ~BaseInstance() {};

	/// nuke thoroughly - deletes the instance contents, notifies the list/model which is
	/// responsible of cleaning up the husk
	void nuke();

	/// The instance's ID. The ID SHALL be determined by MMC internally. The ID IS guaranteed to
	/// be unique.
	virtual QString id() const;

	/// get the type of this instance
	QString instanceType() const;

	/// Path to the instance's root directory.
	QString instanceRoot() const;

	/// Path to the instance's minecraft directory.
	QString minecraftRoot() const;

	QString name() const;
	void setName(QString val);

	/// Value used for instance window titles
	QString windowTitle() const;

	QString iconKey() const;
	void setIconKey(QString val);

	QString notes() const;
	void setNotes(QString val);

	QString group() const;
	void setGroupInitial(QString val);
	void setGroupPost(QString val);

	QStringList extraArguments() const;

	virtual QString intendedVersionId() const = 0;
	virtual bool setIntendedVersionId(QString version) = 0;

	virtual bool versionIsCustom() = 0;

	/*!
	 * The instance's current version.
	 * This value represents the instance's current version. If this value is
	 * different from the intendedVersion, the instance should be updated.
	 * \warning Don't change this value unless you know what you're doing.
	 */
	virtual QString currentVersionId() const = 0;

	/*!
	 * Whether or not Minecraft should be downloaded when the instance is launched.
	 */
	virtual bool shouldUpdate() const = 0;
	virtual void setShouldUpdate(bool val) = 0;

	/// Get the curent base jar of this instance. By default, it's the
	/// versions/$version/$version.jar
	QString baseJar() const;

	/// the default base jar of this instance
	virtual QString defaultBaseJar() const = 0;
	/// the default custom base jar of this instance
	virtual QString defaultCustomBaseJar() const = 0;

	/*!
	 * Whether or not custom base jar is used
	 */
	bool shouldUseCustomBaseJar() const;
	void setShouldUseCustomBaseJar(bool val);
	/*!
	 * The value of the custom base jar
	 */
	QString customBaseJar() const;
	void setCustomBaseJar(QString val);

	/**
	 * Gets the time that the instance was last launched.
	 * Stored in milliseconds since epoch.
	 */
	qint64 lastLaunch() const;
	/// Sets the last launched time to 'val' milliseconds since epoch
	void setLastLaunch(qint64 val = QDateTime::currentMSecsSinceEpoch());

	/*!
	 * \brief Gets the instance list that this instance is a part of.
	 *        Returns NULL if this instance is not in a list
	 *        (the parent is not an InstanceList).
	 * \return A pointer to the InstanceList containing this instance.
	 */
	InstanceList *instList() const;

	/*!
	 * \brief Gets a pointer to this instance's version list.
	 * \return A pointer to the available version list for this instance.
	 */
	virtual std::shared_ptr<BaseVersionList> versionList() const;

	/*!
	 * \brief Gets this instance's settings object.
	 * This settings object stores instance-specific settings.
	 * \return A pointer to this instance's settings object.
	 */
	virtual SettingsObject &settings() const;

	/// returns a valid update task
	virtual std::shared_ptr<Task> doUpdate() = 0;

	/// returns a valid minecraft process, ready for launch with the given account.
	virtual MinecraftProcess *prepareForLaunch(AuthSessionPtr account) = 0;

	/// do any necessary cleanups after the instance finishes. also runs before
	/// 'prepareForLaunch'
	virtual void cleanupAfterRun() = 0;

	/// create a mod edit dialog for the instance
	virtual QDialog *createModEditDialog(QWidget *parent) = 0;

	/// is a particular action enabled with this instance selected?
	virtual bool menuActionEnabled(QString action_name) const = 0;

	virtual QString getStatusbarDescription() = 0;

	/// FIXME: this really should be elsewhere...
	virtual QString instanceConfigFolder() const = 0;

	enum InstanceFlag
	{
		NoFlags = 0x00,
		VersionBrokenFlag = 0x01
	};
	Q_DECLARE_FLAGS(InstanceFlags, InstanceFlag)
	InstanceFlags flags() const;
	void setFlags(const BaseInstance::InstanceFlags flags);

	bool canLaunch() const;

signals:
	/*!
	 * \brief Signal emitted when properties relevant to the instance view change
	 */
	void propertiesChanged(BaseInstance *inst);
	/*!
	 * \brief Signal emitted when groups are affected in any way
	 */
	void groupChanged();
	/*!
	 * \brief The instance just got nuked. Hurray!
	 */
	void nuked(BaseInstance *inst);

	void flagsChanged();

protected slots:
	void iconUpdated(QString key);

protected:
	std::shared_ptr<BaseInstancePrivate> inst_d;
};

// pointer for lazy people
typedef std::shared_ptr<BaseInstance> InstancePtr;

Q_DECLARE_OPERATORS_FOR_FLAGS(BaseInstance::InstanceFlags)
