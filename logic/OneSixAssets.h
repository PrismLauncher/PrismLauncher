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
#include "net/NetJob.h"

class Private;
class ThreadedDeleter;

class OneSixAssets : public QObject
{
	Q_OBJECT
signals:
	void failed();
	void finished();
	void indexStarted();
	void filesStarted();
	void filesProgress(int, int, int);

public
slots:
	void S3BucketFinished();
	void downloadFinished();

public:
	void start();

private:
	ThreadedDeleter *deleter;
	QStringList nuke_whitelist;
	NetJobPtr index_job;
	NetJobPtr files_job;
};
