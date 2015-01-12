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

#pragma once

#include <QDebug>
#include <QString>
#include <QDateTime>

namespace QsLogging
{
class Destination;
enum Level
{
	TraceLevel = 0,
	DebugLevel,
	InfoLevel,
	WarnLevel,
	ErrorLevel,
	FatalLevel,
	UnknownLevel
};

class LoggerImpl; // d pointer
class Logger
{
public:
	static Logger &instance()
	{
		static Logger staticLog;
		return staticLog;
	}

	//! Adds a log message destination. Don't add null destinations.
	void addDestination(Destination *destination);
	//! Removes the given destination from the logger.
	void removeDestination(Destination* destination);
	//! Logging at a level < 'newLevel' will be ignored
	void setLoggingLevel(Level newLevel);
	//! The default level is INFO
	Level loggingLevel() const;
	//! msecs since the logger was initialized
	qint64 timeSinceStart() const;
	//! time when the logger was initialized
	QDateTime timeOfStart() const;


	//! The helper forwards the streaming to QDebug and builds the final
	//! log message.
	class Helper
	{
	public:
		explicit Helper(Level logLevel);
		~Helper();
		QDebug &stream()
		{
			return qtDebug;
		}

	private:
		void writeToLog();

		Level level;
		QString buffer;
		QDebug qtDebug;
	};

private:
	Logger();
	Logger(const Logger &);
	Logger &operator=(const Logger &);
	~Logger();

	void write(const QString &message);

	LoggerImpl *d;
};

} // end namespace

#define QLOG_TRACE()                                                                           \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::TraceLevel)                 \
	QsLogging::Logger::Helper(QsLogging::TraceLevel).stream()
#define QLOG_DEBUG()                                                                           \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::DebugLevel)                 \
	QsLogging::Logger::Helper(QsLogging::DebugLevel).stream()
#define QLOG_INFO()                                                                            \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::InfoLevel)                  \
	QsLogging::Logger::Helper(QsLogging::InfoLevel).stream()
#define QLOG_WARN()                                                                            \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::WarnLevel)                  \
	QsLogging::Logger::Helper(QsLogging::WarnLevel).stream()
#define QLOG_ERROR()                                                                           \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::ErrorLevel)                 \
	QsLogging::Logger::Helper(QsLogging::ErrorLevel).stream()
#define QLOG_FATAL() QsLogging::Logger::Helper(QsLogging::FatalLevel).stream()

/*
#define QLOG_TRACE()                                                                           \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::TraceLevel)                 \
	QsLogging::Logger::Helper(QsLogging::TraceLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_DEBUG()                                                                           \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::DebugLevel)                 \
	QsLogging::Logger::Helper(QsLogging::DebugLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_INFO()                                                                            \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::InfoLevel)                  \
	QsLogging::Logger::Helper(QsLogging::InfoLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_WARN()                                                                            \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::WarnLevel)                  \
	QsLogging::Logger::Helper(QsLogging::WarnLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_ERROR()                                                                           \
	if (QsLogging::Logger::instance().loggingLevel() <= QsLogging::ErrorLevel)                 \
	QsLogging::Logger::Helper(QsLogging::ErrorLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_FATAL()                                                                           \
	QsLogging::Logger::Helper(QsLogging::FatalLevel).stream() << __FILE__ << '@' << __LINE__
*/
