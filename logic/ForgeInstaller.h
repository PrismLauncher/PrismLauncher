#pragma once
#include <QString>
#include <memory>

class OneSixVersion;

class ForgeInstaller
{
public:
	ForgeInstaller(QString filename, QString universal_url);

	bool apply(std::shared_ptr<OneSixVersion> to);

private:
	// the version, read from the installer
	std::shared_ptr<OneSixVersion> m_forge_version;
	QString internalPath;
	QString finalPath;
	QString realVersionId;
	QString m_universal_url;
};




