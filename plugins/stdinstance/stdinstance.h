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

#ifndef STDINSTANCE_H
#define STDINSTANCE_H

#include <instance.h>

class StdInstance : public Instance
{
	Q_OBJECT
public:
	explicit StdInstance(const QString &rootDir, QObject *parent = 0);
	
	virtual bool shouldUpdateCurrentVersion();
	
	virtual void updateCurrentVersion(bool keepCurrent);
	
	////// TIMESTAMPS //////
	virtual qint64 lastVersionUpdate() { return getField("lastVersionUpdate", 0).value<qint64>(); }
	virtual void setLastVersionUpdate(qint64 val) { setField("lastVersionUpdate", val); }
};

#endif // STDINSTANCE_H
