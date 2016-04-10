/* Copyright 2015 MultiMC Contributors
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

#include "BaseConfigObject.h"

#include <QTimer>
#include <QFile>
#include <QCoreApplication>
#include <QDebug>

#include "Exception.h"
#include "FileSystem.h"

BaseConfigObject::BaseConfigObject(const QString &filename)
	: m_filename(filename)
{
	m_saveTimer = new QTimer;
	m_saveTimer->setSingleShot(true);
	// cppcheck-suppress pureVirtualCall
	QObject::connect(m_saveTimer, &QTimer::timeout, [this](){saveNow();});
	setSaveTimeout(250);

	m_initialReadTimer = new QTimer;
	m_initialReadTimer->setSingleShot(true);
	QObject::connect(m_initialReadTimer, &QTimer::timeout, [this]()
	{
		loadNow();
		m_initialReadTimer->deleteLater();
		m_initialReadTimer = 0;
	});
	m_initialReadTimer->start(0);

	// cppcheck-suppress pureVirtualCall
	m_appQuitConnection = QObject::connect(qApp, &QCoreApplication::aboutToQuit, [this](){saveNow();});
}
BaseConfigObject::~BaseConfigObject()
{
	delete m_saveTimer;
	if (m_initialReadTimer)
	{
		delete m_initialReadTimer;
	}
	QObject::disconnect(m_appQuitConnection);
}

void BaseConfigObject::setSaveTimeout(int msec)
{
	m_saveTimer->setInterval(msec);
}

void BaseConfigObject::scheduleSave()
{
	m_saveTimer->stop();
	m_saveTimer->start();
}
void BaseConfigObject::saveNow()
{
	if (m_saveTimer->isActive())
	{
		m_saveTimer->stop();
	}
	if (m_disableSaving)
	{
		return;
	}

	try
	{
		FS::write(m_filename, doSave());
	}
	catch (Exception & e)
	{
		qCritical() << e.cause();
	}
}
void BaseConfigObject::loadNow()
{
	if (m_saveTimer->isActive())
	{
		saveNow();
	}

	try
	{
		doLoad(FS::read(m_filename));
	}
	catch (Exception & e)
	{
		qWarning() << "Error loading" << m_filename << ":" << e.cause();
	}
}
