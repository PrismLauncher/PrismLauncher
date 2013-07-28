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

#ifndef USERINFO_H
#define USERINFO_H

#include <QObject>

#include "libmmc_config.h"

class LIBMULTIMC_EXPORT UserInfo : public QObject
{
	Q_OBJECT
public:
	explicit UserInfo(const QString& username, const QString& password, QObject *parent = 0);
	explicit UserInfo(const UserInfo& other);
	
	QString username() const;
	void setUsername(const QString& username);
	
	QString password() const;
	void setPassword(const QString& password);
	
protected:
	QString m_username;
	QString m_password;
};

#endif // USERINFO_H
