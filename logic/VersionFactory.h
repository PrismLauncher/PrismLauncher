#pragma once
#include <QtCore>

struct OneSixVersion;
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
	QSharedPointer<OneSixVersion> parse(QByteArray data);
private:
	QSharedPointer<OneSixVersion> parse4(QJsonObject root, QSharedPointer<OneSixVersion> product);
	QList<QSharedPointer<Rule> > parse4rules(QJsonObject & baseObj);
};