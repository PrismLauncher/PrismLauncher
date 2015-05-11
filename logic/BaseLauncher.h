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

class BaseLauncher: public QObject
{
	Q_OBJECT
protected:
	explicit BaseLauncher(InstancePtr instance);
	void init();

public: /* methods */
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

	void killProcess();

	qint64 pid();

	/**
	 * @brief prepare the process for launch (for multi-stage launch)
	 */
	virtual void arm() = 0;

	/**
	 * @brief launch the armed instance
	 */
	virtual void launch() = 0;

	/**
	 * @brief abort launch
	 */
	virtual void abort() = 0;

protected: /* methods */
	void preLaunch();
	void postLaunch();
	QString substituteVariables(const QString &cmd) const;
	void initializeEnvironment();

	void printHeader();

	virtual QMap<QString, QString> getVariables() const = 0;
	virtual QString censorPrivateInfo(QString in) = 0;
	virtual MessageLevel::Enum guessLevel(const QString &message, MessageLevel::Enum defaultLevel) = 0;

signals:
	/**
	 * @brief emitted when the Process immediately fails to run
	 */
	void launch_failed(InstancePtr);

	/**
	 * @brief emitted when the PreLaunchCommand fails
	 */
	void prelaunch_failed(InstancePtr, int code, QProcess::ExitStatus status);

	/**
	 * @brief emitted when the PostLaunchCommand fails
	 */
	void postlaunch_failed(InstancePtr, int code, QProcess::ExitStatus status);

	/**
	 * @brief emitted when the process has finished and the PostLaunchCommand was run
	 */
	void ended(InstancePtr, int code, QProcess::ExitStatus status);

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

	bool killed = false;
	QString m_header;
};
