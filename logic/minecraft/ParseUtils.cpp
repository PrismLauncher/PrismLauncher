#include <QDateTime>
#include <QString>
#include "ParseUtils.h"
#include <MMCJson.h>

QDateTime timeFromS3Time(QString str)
{
	return QDateTime::fromString(str, Qt::ISODate);
}

bool parse_timestamp (const QString & raw, QString & save_here, QDateTime & parse_here)
{
	save_here = raw;
	if (save_here.isEmpty())
	{
		return false;
	}
	parse_here = timeFromS3Time(save_here);
	if (!parse_here.isValid())
	{
		return false;
	}
	return true;
}
