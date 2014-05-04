#include "URNResolver.h"
#include <logger/QsLog.h>
#include "MultiMC.h"
#include "logic/forge/ForgeVersionList.h"
#include "logic/forge/ForgeVersion.h"

QString unescapeNSS(QString RawNSS)
{
	QString NSS;
	NSS.reserve(RawNSS.size());
	enum
	{
		Normal,
		FirstHex,
		SecondHex
	} ParseState = Normal;

	QByteArray translator("  ");

	for (auto ch : RawNSS)
	{
		if(ParseState == Normal)
		{
			if(ch == '%')
			{
				ParseState = FirstHex;
				continue;
			}
			else
			{
				NSS.append(ch);
			}
		}
		if(ParseState == FirstHex)
		{
			translator[0] = ch.toLower().unicode();
			ParseState = SecondHex;
		}
		else if(ParseState == SecondHex)
		{
			translator[1] = ch.toLower().unicode();
			auto result = QByteArray::fromHex(translator);
			if (result[0] == '\0')
				return NSS;
			NSS.append(result);
			ParseState = Normal;
		}
	}
	return NSS;
}

bool URNResolver::parse(const QString &URN, QString &NID, QString &NSS)
{
	QRegExp URNPattern(
		"^urn:([a-z0-9][a-z0-9-]{0,31}):(([a-z0-9()+,\\-.:=@;$_!*']|%[0-9a-f]{2})+).*",
		Qt::CaseInsensitive);
	if (URNPattern.indexIn(URN) == -1)
		return false;
	auto captures = URNPattern.capturedTexts();
	QString RawNID = captures[1];
	QString RawNSS = captures[2];

	NID = RawNID.toLower();
	NSS = unescapeNSS(RawNSS);
	return true;
}

URNResolver::URNResolver()
{
}

QVariant URNResolver::resolve(QString URN)
{
	QString NID, NSS;
	parse(URN, NID, NSS);

	if(NID != "x-mmc")
		return QVariant();
	auto parts = NSS.split(":");
	if(parts.size() < 1)
		return QVariant();
	unsigned int version = parts[0].toUInt();
	switch(version)
	{
		case 1:
			return resolveV1(parts.mid(1));
		default:
			return QVariant();
	}
}

/**
 * TODO: implement.
 */
QVariant URNResolver::resolveV1(QStringList parts)
{
	return QVariant();
}
