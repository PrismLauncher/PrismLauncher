#pragma once
#include <QString>
#include <QFileInfo>
#include <QSet>
#include "Mod.h"
#include <functional>

class QuaZip;
namespace JarUtils
{
	bool noFilter(QString);
	bool metaInfFilter(QString key);

	bool mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained,
				   std::function<bool(QString)> filter);

	bool createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods);
}
