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
#include "LaunchStep.h"

class LaunchTask: public Task
{
	Q_OBJECT
protected:
	explicit LaunchTask(InstancePtr instance);
	void init();

public:
	enum State
	{
		NotStarted,
		Running,
		Waiting,
		Failed,
		Aborted,
		Finished
	};

public: /* methods */
	static std::shared_ptr<LaunchTask> create(InstancePtr inst);
	virtual ~LaunchTask() {};

	void appendStep(std::shared_ptr<LaunchStep> step);
	void prependStep(std::shared_ptr<LaunchStep> step);
	void setCensorFilter(QMap<QString, QString> filter);

	InstancePtr instance()
	{
		return m_instance;
	}

	void setPid(qint64 pid)
	{
		m_pid = pid;
	}

	qint64 pid()
	{
		return m_pid;
	}

	/**
	 * @brief prepare the process for launch (for multi-stage launch)
	 */
	virtual void executeTask() override;

	/**
	 * @brief launch the armed instance
	 */
	void proceed();

	/**
	 * @brief abort launch
	 */
	virtual bool abort() override;

public:
	QString substituteVariables(const QString &cmd) const;
	QString censorPrivateInfo(QString in);

protected: /* methods */
	virtual void emitFailed(QString reason);
	virtual void emitSucceeded();

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

public slots:
	void onLogLines(const QStringList& lines, MessageLevel::Enum defaultLevel = MessageLevel::MultiMC);
	void onLogLine(QString line, MessageLevel::Enum defaultLevel = MessageLevel::MultiMC);
	void onReadyForLaunch();
	void onStepFinished();

protected: /* data */
	InstancePtr m_instance;
	QList <std::shared_ptr<LaunchStep>> m_steps;
	QMap<QString, QString> m_censorFilter;
	int currentStep = -1;
	State state = NotStarted;
	qint64 m_pid = -1;
};
