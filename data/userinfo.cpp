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

#include "userinfo.h"

UserInfo::UserInfo(const QString &username, const QString &password, QObject *parent) :
	QObject(parent)
{
	this->username = username;
	this->password = password;
}

UserInfo::UserInfo(const UserInfo &other)
{
	this->username = other.username;
	this->password = other.password;
}

QString UserInfo::getUsername() const
{
	return username;
}

void UserInfo::setUsername(const QString &username)
{
	this->username = username;
}

QString UserInfo::getPassword() const
{
	return password;
}

void UserInfo::setPassword(const QString &password)
{
	this->password = password;
}
