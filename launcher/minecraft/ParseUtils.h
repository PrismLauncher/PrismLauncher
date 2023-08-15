#pragma once
#include <QDateTime>
#include <QString>

/// take the timestamp used by S3 and turn it into QDateTime
QDateTime timeFromS3Time(QString str);

/// take a timestamp and convert it into an S3 timestamp
QString timeToS3Time(QDateTime);
