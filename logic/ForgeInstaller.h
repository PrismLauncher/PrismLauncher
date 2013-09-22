#pragma once
#include <QString>
#include <QSharedPointer>

class OneSixVersion;

class ForgeInstaller
{
public:
	ForgeInstaller(QString filename, QString universal_url);

	bool apply(QSharedPointer<OneSixVersion> to);

private:
	// the version, read from the installer
	QSharedPointer<OneSixVersion> m_forge_version;
	QString internalPath;
	QString finalPath;
	QString realVersionId;
	QString m_universal_url;
};




