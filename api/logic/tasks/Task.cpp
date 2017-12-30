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

#include "Task.h"

#include <QDebug>

Task::Task(QObject *parent) : QObject(parent)
{
}

void Task::setStatus(const QString &new_status)
{
	if(m_status != new_status)
	{
		m_status = new_status;
		emit status(m_status);
	}
}

void Task::setProgress(qint64 current, qint64 total)
{
	m_progress = current;
	m_progressTotal = total;
	emit progress(m_progress, m_progressTotal);
}

void Task::start()
{
	m_running = true;
	emit started();
	qDebug() << "Task" << describe() << "started";
	executeTask();
}

void Task::emitFailed(QString reason)
{
	// Don't fail twice.
	if (!m_running)
	{
		qCritical() << "Task" << describe() << "failed while not running!!!!: " << reason;
		return;
	}
	m_running = false;
	m_finished = true;
	m_succeeded = false;
	m_failReason = reason;
	qCritical() << "Task" << describe() << "failed: " << reason;
	emit failed(reason);
	emit finished();
}

void Task::emitAborted()
{
	// Don't abort twice.
	if (!m_running)
	{
		qCritical() << "Task" << describe() << "aborted while not running!!!!";
		return;
	}
	m_running = false;
	m_finished = true;
	m_succeeded = false;
	m_failReason = "Aborted.";
	qDebug() << "Task" << describe() << "aborted.";
	emit failed(m_failReason);
	emit finished();
}

void Task::emitSucceeded()
{
	// Don't succeed twice.
	if (!m_running)
	{
		qCritical() << "Task" << describe() << "succeeded while not running!!!!";
		return;
	}
	m_running = false;
	m_finished = true;
	m_succeeded = true;
	qDebug() << "Task" << describe() << "succeeded";
	emit succeeded();
	emit finished();
}

QString Task::describe()
{
	QString outStr;
	QTextStream out(&outStr);
	out << metaObject()->className() << QChar('(');
	auto name = objectName();
	if(name.isEmpty())
	{
		out << QString("0x%1").arg((quintptr)this, 0, 16);
	}
	else
	{
		out << name;
	}
	out << QChar(')');
	out.flush();
	return outStr;
}

bool Task::isRunning() const
{
	return m_running;
}

bool Task::isFinished() const
{
	return m_finished;
}

bool Task::wasSuccessful() const
{
	return m_succeeded;
}

QString Task::failReason() const
{
	return m_failReason;
}

void Task::logWarning(const QString& line)
{
	qWarning() << line;
	m_Warnings.append(line);
}

QStringList Task::warnings() const
{
	return m_Warnings;
}
