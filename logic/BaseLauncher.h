/* Copyright 2013-2015 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <QProcess>
#include "BaseInstance.h"
#include "MessageLevel.h"
#include "LoggedProcess.h"
/* HACK: MINECRAFT: split! */
#include "minecraft/MinecraftInstance.h"
#include "java/JavaChecker.h"
#include "QObjectPtr.h"
#include "tasks/Task.h"

class ProcessTask
{

};

class BaseProfilerFactory;
class BaseLauncher: public Task
{
	Q_OBJECT
protected:
	explicit BaseLauncher(InstancePtr instance);
	void init();

public: /* methods */
	static std::shared_ptr<BaseLauncher> create(MinecraftInstancePtr inst);
	virtual ~BaseLauncher() {};

	InstancePtr instance()
	{
		return m_instance;
	}

	/// Set the text printed on top of the log
	void setHeader(QString header)
	{
		m_header = header;
	}

	void setWorkdir(QString path);

	BaseProfilerFactory * getProfiler()
	{
		return m_profiler;
	}

	void setProfiler(BaseProfilerFactory * profiler)
	{
		m_profiler = profiler;
	}

	void killProcess();

	qint64 pid();

	/**
	 * @brief prepare the process for launch (for multi-stage launch)
	 */
	virtual void executeTask() override;

	/**
	 * @brief launch the armed instance
	 */
	virtual void launch();

	/**
	 * @brief abort launch
	 */
	virtual void abort();

public: /* HACK: MINECRAFT: split! */
	void setLaunchScript(QString script)
	{
		launchScript = script;
	}

	void setNativeFolder(QString natives)
	{
		m_nativeFolder = natives;
	}

	inline void setLogin(AuthSessionPtr session)
	{
		m_session = session;
	}


protected: /* methods */
	void preLaunch();
	void updateInstance();
	void makeReady();
	void postLaunch();
	virtual void emitFailed(QString reason);
	virtual void emitSucceeded();
	QString substituteVariables(const QString &cmd) const;
	void initializeEnvironment();

	void printHeader();

	virtual QMap<QString, QString> getVariables() const;
	virtual QString censorPrivateInfo(QString in);
	virtual MessageLevel::Enum guessLevel(const QString &message, MessageLevel::Enum defaultLevel);

signals:
	/**
	 * @brief emitted when the launch preparations are done
	 */
	void readyForLaunch();

	/**
	 * @brief emitted when we want to log something
	 * @param text the text to log
	 * @param level the level to log at
	 */
	void log(QString text, MessageLevel::Enum level = MessageLevel::MultiMC);

protected slots:
	void on_log(QStringList lines, MessageLevel::Enum level);
	void logOutput(const QStringList& lines, MessageLevel::Enum defaultLevel = MessageLevel::Message);
	void logOutput(QString line, MessageLevel::Enum defaultLevel = MessageLevel::Message);

	void on_pre_state(LoggedProcess::State state);
	void on_state(LoggedProcess::State state);
	void on_post_state(LoggedProcess::State state);



protected:
	InstancePtr m_instance;

	LoggedProcess m_prelaunchprocess;
	LoggedProcess m_postlaunchprocess;
	LoggedProcess m_process;
	QProcessEnvironment m_env;
	BaseProfilerFactory * m_profiler = nullptr;

	bool killed = false;
	QString m_header;

/**
 * java check step
 */
protected slots:
	void checkJavaFinished(JavaCheckResult result);

protected:
	// for java checker and launch
	QString m_javaPath;
	qlonglong m_javaUnixTime;
	std::shared_ptr<JavaChecker> m_JavaChecker;

protected: /* HACK: MINECRAFT: split! */
	AuthSessionPtr m_session;
	QString launchScript;
	QString m_nativeFolder;
	std::shared_ptr<Task> m_updateTask;

protected: /* HACK: MINECRAFT: split! */
	void checkJava();
	QStringList javaArguments() const;
private slots:
	void updateFinished();
};

class BaseProfilerFactory;