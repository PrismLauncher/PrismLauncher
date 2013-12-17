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

#include "QsLogDest.h"
#include "QsDebugOutput.h"
#include "QsLog.h"
#include <QFile>
#include <QTextStream>
#include <QString>

namespace QsLogging
{

Destination::~Destination()
{
	Logger::instance().removeDestination(this);
	QsDebugOutput::output("Removed logger destination.");
}

//! file message sink
class FileDestination : public Destination
{
public:
	FileDestination(const QString &filePath);
	virtual void write(const QString &message);

private:
	QFile mFile;
	QTextStream mOutputStream;
};

FileDestination::FileDestination(const QString &filePath)
{
	mFile.setFileName(filePath);
	mFile.open(QFile::WriteOnly | QFile::Text |
			   QFile::Truncate); // fixme: should throw on failure
	mOutputStream.setDevice(&mFile);
}

void FileDestination::write(const QString &message)
{
	mOutputStream << message << endl;
	mOutputStream.flush();
}

//! debugger sink
class DebugOutputDestination : public Destination
{
public:
	virtual void write(const QString &message);
};

void DebugOutputDestination::write(const QString &message)
{
	QsDebugOutput::output(message);
}

class QDebugDestination : public Destination
{
public:
	virtual void write(const QString &message)
	{
		qDebug() << message;
	};
};

DestinationPtr DestinationFactory::MakeFileDestination(const QString &filePath)
{
	return DestinationPtr(new FileDestination(filePath));
}

DestinationPtr DestinationFactory::MakeDebugOutputDestination()
{
	return DestinationPtr(new DebugOutputDestination);
}

DestinationPtr DestinationFactory::MakeQDebugDestination()
{
	return DestinationPtr(new QDebugDestination);
}

} // end namespace
