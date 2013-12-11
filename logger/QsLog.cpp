// Copyright (c) 2010, Razvan Petru
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or other
//   materials provided with the distribution.
// * The name of the contributors may not be used to endorse or promote products
//   derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "QsLog.h"
#include "QsLogDest.h"
#include <QMutex>
#include <QList>
#include <QDateTime>
#include <QtGlobal>
#include <cassert>
#include <cstdlib>
#include <stdexcept>

namespace QsLogging
{
typedef QList<Destination *> DestinationList;

static const char *LevelStrings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
																				"UNKNOWN"};

// not using Qt::ISODate because we need the milliseconds too
static const QString fmtDateTime("hhhh:mm:ss.zzz");

static const char *LevelToText(Level theLevel)
{
	if (theLevel > FatalLevel)
	{
		assert(!"bad log level");
		return LevelStrings[UnknownLevel];
	}
	return LevelStrings[theLevel];
}

class LoggerImpl
{
public:
	LoggerImpl() : level(InfoLevel)
	{
	}
	QMutex logMutex;
	Level level;
	DestinationList destList;
};

Logger::Logger() : d(new LoggerImpl)
{
}

Logger::~Logger()
{
	delete d;
}

void Logger::addDestination(Destination *destination)
{
	assert(destination);
	d->destList.push_back(destination);
}

void Logger::setLoggingLevel(Level newLevel)
{
	d->level = newLevel;
}

Level Logger::loggingLevel() const
{
	return d->level;
}

//! creates the complete log message and passes it to the logger
void Logger::Helper::writeToLog()
{
	const char *const levelName = LevelToText(level);
	const QString completeMessage(QString("%1\t%2").arg(levelName, 5).arg(buffer));

	Logger &logger = Logger::instance();
	QMutexLocker lock(&logger.d->logMutex);
	logger.write(completeMessage);
}

Logger::Helper::Helper(Level logLevel) : level(logLevel), qtDebug(&buffer)
{
}

Logger::Helper::~Helper()
{
	try
	{
		writeToLog();
	}
	catch (std::exception &e)
	{
		// you shouldn't throw exceptions from a sink
		Q_UNUSED(e);
		assert(!"exception in logger helper destructor");
		throw;
	}
}

//! sends the message to all the destinations
void Logger::write(const QString &message)
{
	for (DestinationList::iterator it = d->destList.begin(), endIt = d->destList.end();
		 it != endIt; ++it)
	{
		if (!(*it))
		{
			assert(!"null log destination");
			continue;
		}
		(*it)->write(message);
	}
}

void Logger::removeDestination(Destination* destination)
{
	d->destList.removeAll(destination);
}

} // end namespace
