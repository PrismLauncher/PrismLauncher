#pragma once
#include <QString>
#include <QJsonObject>
#include <memory>
class Jarmod;
typedef std::shared_ptr<Jarmod> JarmodPtr;
class Jarmod
{
public: /* data */
	QString name;
	QString originalName;
};
