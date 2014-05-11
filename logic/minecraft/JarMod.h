#pragma once
#include <QString>
#include <QJsonObject>
#include <memory>
class Jarmod;
typedef std::shared_ptr<Jarmod> JarmodPtr;
class Jarmod
{
public: /* methods */
	static JarmodPtr fromJson(const QJsonObject &libObj, const QString &filename);
	QJsonObject toJson();
	QString url();
public: /* data */
	QString name;
	QString baseurl;
	QString hint;
	QString absoluteUrl;
};
