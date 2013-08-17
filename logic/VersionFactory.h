#pragma once
#include <QtCore>

struct FullVersion;
class Rule;

class FullVersionFactory
{
public:
	enum Error
	{
		AllOK, // all parsed OK
		ParseError, // the file was corrupted somehow
		UnsupportedVersion // the file was meant for a launcher version we don't support (yet)
	} m_error;
	QString error_string;

public:
	FullVersionFactory();
	QSharedPointer<FullVersion> parse(QByteArray data);
private:
	QSharedPointer<FullVersion> parse4(QJsonObject root, QSharedPointer<FullVersion> product);
	QList<QSharedPointer<Rule> > parse4rules(QJsonObject & baseObj);
};