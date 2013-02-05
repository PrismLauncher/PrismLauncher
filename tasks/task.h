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

#ifndef TASK_H
#define TASK_H

#include <QObject>
#include <QThread>
#include <QString>

class Task : public QThread
{
	Q_OBJECT
public:
	explicit Task(QObject *parent = 0);
	
	// Starts the task.
	void startTask();
	
	QString getStatus() const;
	int getProgress() const;
	
signals:
	void taskStarted(Task* task);
	void taskEnded(Task* task);
	
	void statusChanged(const QString& status);
	void progressChanged(int progress);
	
protected:
	void setStatus(const QString& status);
	void setProgress(int progress);
	
	virtual void run();
	virtual void executeTask() = 0;
	
	QString status;
	int progress;
};

#endif // TASK_H
