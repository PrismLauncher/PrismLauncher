#pragma once
#include <exception>
#include <QString>
#include <QDebug>

class MMCError : public std::exception
{
public:
	MMCError(QString cause)
	{
		exceptionCause = cause;
		qCritical() << "Exception: " + cause;
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
