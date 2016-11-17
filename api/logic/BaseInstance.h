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
#include <cassert>

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>

#include "settings/SettingsObject.h"

#include "settings/INIFile.h"
#include "BaseVersionList.h"
#include "minecraft/auth/MojangAccount.h"
#include "MessageLevel.h"
#include "pathmatcher/IPathMatcher.h"

#include "multimc_logic_export.h"

class QDir;
class Task;
class LaunchTask;
class BaseInstance;
class BaseInstanceProvider;

// pointer for lazy people
typedef std::shared_ptr<BaseInstance> InstancePtr;

/*!
 * \brief Base class for instances.
 * This class implements many functions that are common between instances and
 * provides a standard interface for all instances.
 *
 * To create a new instance type, create a new class inheriting from this class
 * and implement the pure virtual functions.
 */
class MULTIMC_LOGIC_EXPORT BaseInstance : public QObject, public std::enable_shared_from_this<BaseInstance>
{
	Q_OBJECT
protected:
	/// no-touchy!
	BaseInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);

public: /* types */
	enum class Status
	{
		Present,
		Gone // either nuked or invalidated
	};

public:
	/// virtual destructor to make sure the destruction is COMPLETE
	virtual ~BaseInstance() {};

	virtual void copy(const QDir &newDir) {}

	virtual void init() = 0;

	/// nuke thoroughly - deletes the instance contents, notifies the list/model which is
	/// responsible of cleaning up the husk
	void nuke();

	/***
	 * the instance has been invalidated - it is no longer tracked by MultiMC for some reason,
	 * but it has not necessarily been deleted.
	 *
	 * Happens when the instance folder changes to some other location, or the instance is removed by external means.
	 */
	void invalidate();

	/// The instance's ID. The ID SHALL be determined by MMC internally. The ID IS guaranteed to
	/// be unique.
	virtual QString id() const;

	void setRunning(bool running);
	bool isRunning() const;
	int64_t totalTimePlayed() const;
	void resetTimePlayed();

	void setProvider(BaseInstanceProvider * provider);
	BaseInstanceProvider * provider() const;

	/// get the type of this instance
	QString instanceType() const;

	/// Path to the instance's root directory.
	QString instanceRoot() const;

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

	QString getPreLaunchCommand();
	QString getPostExitCommand();
	QString getWrapperCommand();

	/// guess log level from a line of game log
	virtual MessageLevel::Enum guessLevel(const QString &line, MessageLevel::Enum level)
	{
		return level;
	};

	virtual QStringList extraArguments() const;

	virtual QString intendedVersionId() const = 0;
	virtual bool setIntendedVersionId(QString version) = 0;

	/*!
	 * The instance's current version.
	 * This value represents the instance's current version. If this value is
	 * different from the intendedVersion, the instance should be updated.
	 * \warning Don't change this value unless you know what you're doing.
	 */
	virtual QString currentVersionId() const = 0;

	/*!
	 * Whether or not 'the game' should be downloaded when the instance is launched.
	 */
	virtual bool shouldUpdate() const = 0;
	virtual void setShouldUpdate(bool val) = 0;

	/// Traits. Normally inside the version, depends on instance implementation.
	virtual QSet <QString> traits() = 0;

	/**
	 * Gets the time that the instance was last launched.
	 * Stored in milliseconds since epoch.
	 */
	qint64 lastLaunch() const;
	/// Sets the last launched time to 'val' milliseconds since epoch
	void setLastLaunch(qint64 val = QDateTime::currentMSecsSinceEpoch());

	InstancePtr getSharedPtr();

	/*!
	 * \brief Gets a pointer to this instance's version list.
	 * \return A pointer to the available version list for this instance.
	 */
	virtual std::shared_ptr<BaseVersionList> versionList() const = 0;

	/*!
	 * \brief Gets this instance's settings object.
	 * This settings object stores instance-specific settings.
	 * \return A pointer to this instance's settings object.
	 */
	virtual SettingsObjectPtr settings() const;

	/// returns a valid update task
	virtual shared_qobject_ptr<Task> createUpdateTask() = 0;

	/// returns a valid launcher (task container)
	virtual std::shared_ptr<LaunchTask> createLaunchTask(AuthSessionPtr account) = 0;

	/// returns the current launch task (if any)
	std::shared_ptr<LaunchTask> getLaunchTask();

	/*!
	 * Returns a task that should be done right before launch
	 * This task should do any extra preparations needed
	 */
	virtual std::shared_ptr<Task> createJarModdingTask() = 0;

	/*!
	 * Create envrironment variables for running the instance
	 */
	virtual QProcessEnvironment createEnvironment() = 0;

	/*!
	 * Returns a matcher that can maps relative paths within the instance to whether they are 'log files'
	 */
	virtual IPathMatcher::Ptr getLogFileMatcher() = 0;

	/*!
	 * Returns the root folder to use for looking up log files
	 */
	virtual QString getLogFileRoot() = 0;

	virtual QString getStatusbarDescription() = 0;

	/// FIXME: this really should be elsewhere...
	virtual QString instanceConfigFolder() const = 0;

	/// get variables this instance exports
	virtual QMap<QString, QString> getVariables() const = 0;

	virtual QString typeName() const = 0;

	bool hasVersionBroken() const
	{
		return m_hasBrokenVersion;
	}
	void setVersionBroken(bool value)
	{
		if(m_hasBrokenVersion != value)
		{
			m_hasBrokenVersion = value;
			emit propertiesChanged(this);
		}
	}

	bool hasUpdateAvailable() const
	{
		return m_hasUpdate;
	}
	void setUpdateAvailable(bool value)
	{
		if(m_hasUpdate != value)
		{
			m_hasUpdate = value;
			emit propertiesChanged(this);
		}
	}

	bool hasCrashed() const
	{
		return m_crashed;
	}
	void setCrashed(bool value)
	{
		if(m_crashed != value)
		{
			m_crashed = value;
			emit propertiesChanged(this);
		}
	}

	bool canLaunch() const;
	virtual bool canExport() const = 0;

	virtual bool reload();

	/**
	 * 'print' a verbose desription of the instance into a QStringList
	 */
	virtual QStringList verboseDescription(AuthSessionPtr session) = 0;

	Status currentStatus() const;

protected:
	void changeStatus(Status newStatus);

signals:
	/*!
	 * \brief Signal emitted when properties relevant to the instance view change
	 */
	void propertiesChanged(BaseInstance *inst);
	/*!
	 * \brief Signal emitted when groups are affected in any way
	 */
	void groupChanged();

	void launchTaskChanged(std::shared_ptr<LaunchTask>);

	void runningStatusChanged(bool running);

	void statusChanged(Status from, Status to);

protected slots:
	void iconUpdated(QString key);

protected: /* data */
	QString m_rootDir;
	QString m_group;
	SettingsObjectPtr m_settings;
	// InstanceFlags m_flags;
	bool m_isRunning = false;
	std::shared_ptr<LaunchTask> m_launchProcess;
	QDateTime m_timeStarted;
	BaseInstanceProvider * m_provider = nullptr;

private: /* data */
	Status m_status = Status::Present;
	bool m_crashed = false;
	bool m_hasUpdate = false;
	bool m_hasBrokenVersion = false;
};

Q_DECLARE_METATYPE(std::shared_ptr<BaseInstance>)
//Q_DECLARE_METATYPE(BaseInstance::InstanceFlag)
//Q_DECLARE_OPERATORS_FOR_FLAGS(BaseInstance::InstanceFlags)
