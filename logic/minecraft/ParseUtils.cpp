#include <QDateTime>
#include <QString>
#include "ParseUtils.h"
#include <logic/MMCJson.h>

QDateTime timeFromS3Time(QString str)
{
	return QDateTime::fromString(str, Qt::ISODate);
}

void parse_timestamp (const QString & raw, QString & save_here, QDateTime & parse_here)
{
	save_here = raw;
	if (save_here.isEmpty())
	{
		throw JSONValidationError("The timestamp is empty!");
	}
	parse_here = timeFromS3Time(save_here);
	if (!parse_here.isValid())
	{
		throw JSONValidationError("The timestamp not a valid timestamp!");
	}
}
