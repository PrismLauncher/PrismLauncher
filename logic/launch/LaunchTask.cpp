/* Copyright 2013-2015 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "launch/LaunchTask.h"
#include "MessageLevel.h"
#include "MMCStrings.h"
#include "java/JavaChecker.h"
#include "tasks/Task.h"
#include <pathutils.h>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QStandardPaths>
#include <assert.h>

void LaunchTask::init()
{
	m_instance->setRunning(true);
}

std::shared_ptr<LaunchTask> LaunchTask::create(InstancePtr inst)
{
	std::shared_ptr<LaunchTask> proc(new LaunchTask(inst));
	proc->init();
	return proc;
}

LaunchTask::LaunchTask(InstancePtr instance): m_instance(instance)
{
}

void LaunchTask::appendStep(std::shared_ptr<LaunchStep> step)
{
	m_steps.append(step);
}

void LaunchTask::prependStep(std::shared_ptr<LaunchStep> step)
{
	m_steps.prepend(step);
}

void LaunchTask::executeTask()
{
	if(!m_steps.size())
	{
		state = LaunchTask::Finished;
		emitSucceeded();
	}
	state = LaunchTask::Running;
	onStepFinished();
}

void LaunchTask::onReadyForLaunch()
{
	state = LaunchTask::Waiting;
	emit readyForLaunch();
}

void LaunchTask::onStepFinished()
{
	// initial -> just start the first step
	if(currentStep == -1)
	{
		currentStep ++;
		m_steps[currentStep]->start();
		return;
	}

	auto step = m_steps[currentStep];
	if(step->successful())
	{
		// end?
		if(currentStep == m_steps.size() - 1)
		{
			emitSucceeded();
		}
		else
		{
			currentStep ++;
			step = m_steps[currentStep];
			step->start();
		}
	}
	else
	{
		emitFailed(step->failReason());
	}
}

void LaunchTask::setCensorFilter(QMap<QString, QString> filter)
{
	m_censorFilter = filter;
}

QString LaunchTask::censorPrivateInfo(QString in)
{
	auto iter = m_censorFilter.begin();
	while (iter != m_censorFilter.end())
	{
		in.replace(iter.key(), iter.value());
		iter++;
	}
	return in;
}

// console window
MessageLevel::Enum LaunchTask::guessLevel(const QString &line, MessageLevel::Enum level)
{
	QRegularExpression re("\\[(?<timestamp>[0-9:]+)\\] \\[[^/]+/(?<level>[^\\]]+)\\]");
	auto match = re.match(line);
	if(match.hasMatch())
	{
		// New style logs from log4j
		QString timestamp = match.captured("timestamp");
		QString levelStr = match.captured("level");
		if(levelStr == "INFO")
			level = MessageLevel::Message;
		if(levelStr == "WARN")
			level = MessageLevel::Warning;
		if(levelStr == "ERROR")
			level = MessageLevel::Error;
		if(levelStr == "FATAL")
			level = MessageLevel::Fatal;
		if(levelStr == "TRACE" || levelStr == "DEBUG")
			level = MessageLevel::Debug;
	}
	else
	{
		// Old style forge logs
		if (line.contains("[INFO]") || line.contains("[CONFIG]") || line.contains("[FINE]") ||
			line.contains("[FINER]") || line.contains("[FINEST]"))
			level = MessageLevel::Message;
		if (line.contains("[SEVERE]") || line.contains("[STDERR]"))
			level = MessageLevel::Error;
		if (line.contains("[WARNING]"))
			level = MessageLevel::Warning;
		if (line.contains("[DEBUG]"))
			level = MessageLevel::Debug;
	}
	if (line.contains("overwriting existing"))
		return MessageLevel::Fatal;
	if (line.contains("Exception in thread") || line.contains(QRegularExpression("\\s+at ")))
		return MessageLevel::Error;
	return level;
}

void LaunchTask::proceed()
{
	if(state != LaunchTask::Waiting)
	{
		return;
	}
	m_steps[currentStep]->proceed();
}

bool LaunchTask::abort()
{
	switch(state)
	{
		case LaunchTask::Aborted:
		case LaunchTask::Failed:
		case LaunchTask::Finished:
			return true;
		case LaunchTask::NotStarted:
		{
			state = LaunchTask::Aborted;
			emitFailed("Aborted");
			return true;
		}
		case LaunchTask::Running:
		case LaunchTask::Waiting:
		{
			auto step = m_steps[currentStep];
			if(!step->canAbort())
			{
				return false;
			}
			if(step->abort())
			{
				state = LaunchTask::Aborted;
				return true;
			}
		}
		default:
			break;
	}
	return false;
}

void LaunchTask::onLogLines(const QStringList &lines, MessageLevel::Enum defaultLevel)
{
	for (auto & line: lines)
	{
		onLogLine(line, defaultLevel);
	}
}

void LaunchTask::onLogLine(QString line, MessageLevel::Enum level)
{
	// if the launcher part set a log level, use it
	auto innerLevel = MessageLevel::fromLine(line);
	if(innerLevel != MessageLevel::Unknown)
	{
		level = innerLevel;
	}

	// If the level is still undetermined, guess level
	if (level == MessageLevel::StdErr || level == MessageLevel::StdOut || level == MessageLevel::Unknown)
	{
		level = this->guessLevel(line, level);
	}

	// censor private user info
	line = censorPrivateInfo(line);

	emit log(line, level);
}

void LaunchTask::emitSucceeded()
{
	m_instance->cleanupAfterRun();
	m_instance->setRunning(false);
	Task::emitSucceeded();
}

void LaunchTask::emitFailed(QString reason)
{
	m_instance->cleanupAfterRun();
	m_instance->setRunning(false);
	Task::emitFailed(reason);
}

QString LaunchTask::substituteVariables(const QString &cmd) const
{
	QString out = cmd;
	auto variables = m_instance->getVariables();
	for (auto it = variables.begin(); it != variables.end(); ++it)
	{
		out.replace("$" + it.key(), it.value());
	}
	auto env = QProcessEnvironment::systemEnvironment();
	for (auto var : env.keys())
	{
		out.replace("$" + var, env.value(var));
	}
	return out;
}

