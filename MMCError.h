#pragma once
#include <exception>
#include <QString>
#include <logger/QsLog.h>

class MMCError : public std::exception
{
public:
	MMCError(QString cause)
	{
		exceptionCause = cause;
		QLOG_ERROR() << "Exception: " + cause;
	};
	virtual ~MMCError() noexcept {}
	virtual const char *what() const noexcept
	{
		return exceptionCause.toLocal8Bit();
	};
	virtual QString cause() const
	{
		return exceptionCause;
	}
private:
	QString exceptionCause;
};
