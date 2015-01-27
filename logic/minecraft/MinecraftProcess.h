/* Copyright 2013-2014 MultiMC Contributors
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

#include <QString>
#include "logic/minecraft/MinecraftInstance.h"
#include "logic/BaseProcess.h"

/**
 * The MinecraftProcess class
 */
class MinecraftProcess : public BaseProcess
{
	Q_OBJECT
protected:
	MinecraftProcess(MinecraftInstancePtr inst);
public:
	static MinecraftProcess *create(MinecraftInstancePtr inst);

	virtual ~MinecraftProcess(){};

	/**
	 * @brief start the launcher part with the provided launch script
	 */
	void arm() override;

	/**
	 * @brief launch the armed instance!
	 */
	void launch() override;

	/**
	 * @brief abort launch!
	 */
	void abort() override;

	void setLaunchScript(QString script)
	{
		launchScript = script;
	}

	void setNativeFolder(QString natives)
	{
		m_nativeFolder = natives;
	}

	inline void setLogin(AuthSessionPtr session)
	{
		m_session = session;
	}

protected:
	AuthSessionPtr m_session;
	QString launchScript;
	QString m_nativeFolder;

	virtual QMap<QString, QString> getVariables() const override;

	QStringList javaArguments() const;
	virtual QString censorPrivateInfo(QString in) override;
	virtual MessageLevel::Enum guessLevel(const QString &message, MessageLevel::Enum defaultLevel) override;
};
