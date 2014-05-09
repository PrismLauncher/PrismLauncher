#pragma once
#include <QString>
#include <QDateTime>

/**
 * parse the S3 timestamp in 'raw' and fill the forwarded variables.
 * return true/false for success/failure
 */
void parse_timestamp (const QString &raw, QString &save_here, QDateTime &parse_here);

/**
 * take the timestamp used by S3 and turn it into QDateTime
 */
QDateTime timeFromS3Time(QString str);
