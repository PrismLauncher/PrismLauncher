#pragma once

#include <QApplication>
#include "MultiMCVersion.h"
#include "config.h"
#include <memory>
#include "logger/QsLog.h"
#include "logger/QsLogDest.h"


class MinecraftVersionList;
class LWJGLVersionList;
class HttpMetaCache;
class SettingsObject;
class InstanceList;
class IconList;
class QNetworkAccessManager;
class ForgeVersionList;

#if defined(MMC)
#undef MMC
#endif
#define MMC (static_cast<MultiMC *>(QCoreApplication::instance()))

class MultiMC : public QApplication
{
	Q_OBJECT
public:
	enum Status
	{
		Failed,
		Succeeded,
		Initialized,
	};

public:
	MultiMC(int &argc, char **argv);
	virtual ~MultiMC();

	std::shared_ptr<SettingsObject> settings()
	{
		return m_settings;
	}

	std::shared_ptr<InstanceList> instances()
	{
		return m_instances;
	}

	std::shared_ptr<IconList> icons();

	Status status()
	{
		return m_status;
	}

	MultiMCVersion version()
	{
		return m_version;
	}

	std::shared_ptr<QNetworkAccessManager> qnam()
	{
		return m_qnam;
	}

	std::shared_ptr<HttpMetaCache> metacache()
	{
		return m_metacache;
	}

	std::shared_ptr<LWJGLVersionList> lwjgllist();

	std::shared_ptr<ForgeVersionList> forgelist();

	std::shared_ptr<MinecraftVersionList> minecraftlist();

private:
	void initLogger();

	void initGlobalSettings();

	void initHttpMetaCache();

	void initTranslations();

private:
	std::shared_ptr<QTranslator> m_qt_translator;
	std::shared_ptr<QTranslator> m_mmc_translator;
	std::shared_ptr<SettingsObject> m_settings;
	std::shared_ptr<InstanceList> m_instances;
	std::shared_ptr<IconList> m_icons;
	std::shared_ptr<QNetworkAccessManager> m_qnam;
	std::shared_ptr<HttpMetaCache> m_metacache;
	std::shared_ptr<LWJGLVersionList> m_lwjgllist;
	std::shared_ptr<ForgeVersionList> m_forgelist;
	std::shared_ptr<MinecraftVersionList> m_minecraftlist;
	QsLogging::DestinationPtr m_fileDestination;
	QsLogging::DestinationPtr m_debugDestination;

	Status m_status = MultiMC::Failed;
	MultiMCVersion m_version = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD};
};
