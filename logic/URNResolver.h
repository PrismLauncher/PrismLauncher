#pragma once
#include <QString>
#include <QMap>
#include <memory>
#include <QVariant>

class URNResolver;
typedef std::shared_ptr<URNResolver> URNResolverPtr;

class URNResolver
{
public:
	URNResolver();
	QVariant resolve (QString URN);
	static bool parse (const QString &URN, QString &NID, QString &NSS);
private:
	QVariant resolveV1 (QStringList parts);
};
