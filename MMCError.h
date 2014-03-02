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
		QLOG_ERROR() << errorName() + ": " + cause;
	};
	virtual ~MMCError(){};
	virtual const char *what() const noexcept
	{
		return exceptionCause.toLocal8Bit();
	};
	virtual QString cause() const
	{
		return exceptionCause;
	}
	virtual QString errorName()
	{
		return "MultiMC Error";
	}
private:
	QString exceptionCause;
};