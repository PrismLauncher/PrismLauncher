#include <QDateTime>
#include <QString>
#include "ParseUtils.h"
#include <QDebug>
#include <cstdlib>

QDateTime timeFromS3Time(QString str)
{
	return QDateTime::fromString(str, Qt::ISODate);
}

QString timeToS3Time(QDateTime time)
{
	// this all because Qt can't format timestamps right.
	int offsetRaw = time.offsetFromUtc();
	bool negative = offsetRaw < 0;
	int offsetAbs = std::abs(offsetRaw);

	int offsetSeconds = offsetAbs % 60;
	offsetAbs -= offsetSeconds;

	int offsetMinutes = offsetAbs % 3600;
	offsetAbs -= offsetMinutes;
	offsetMinutes /= 60;
	
	int offsetHours = offsetAbs / 3600;

	QString raw = time.toString("yyyy-MM-ddTHH:mm:ss");
	raw += (negative ? QChar('-') : QChar('+'));
	raw += QString("%1").arg(offsetHours, 2, 10, QChar('0'));
	raw += ":";
	raw += QString("%1").arg(offsetMinutes, 2, 10, QChar('0'));
	return raw;
}
