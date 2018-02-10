/* Copyright 2013-2018 MultiMC Contributors
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

#include <QtNetwork>
#include "JavaChecker.h"
#include "tasks/Task.h"

class JavaCheckerJob;
typedef std::shared_ptr<JavaCheckerJob> JavaCheckerJobPtr;

// FIXME: this just seems horribly redundant
class JavaCheckerJob : public Task
{
	Q_OBJECT
public:
	explicit JavaCheckerJob(QString job_name) : Task(), m_job_name(job_name) {};

	bool addJavaCheckerAction(JavaCheckerPtr base)
	{
		javacheckers.append(base);
		// if this is already running, the action needs to be started right away!
		if (isRunning())
		{
			setProgress(num_finished, javacheckers.size());
			connect(base.get(), &JavaChecker::checkFinished, this, &JavaCheckerJob::partFinished);
			base->performCheck();
		}
		return true;
	}
	QList<JavaCheckResult> getResults()
	{
		return javaresults;
	}

private slots:
	void partFinished(JavaCheckResult result);

protected:
	virtual void executeTask() override;

private:
	QString m_job_name;
	QList<JavaCheckerPtr> javacheckers;
	QList<JavaCheckResult> javaresults;
	int num_finished = 0;
};
