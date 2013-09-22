#pragma once

#include <QApplication>
#include <QSharedPointer>
#include "MultiMCVersion.h"
#include "config.h"

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
	MultiMC ( int& argc, char** argv );
	virtual ~MultiMC();
	
	QSharedPointer<SettingsObject> settings()
	{
		return m_settings;
	};
	
	QSharedPointer<InstanceList> instances()
	{
		return m_instances;
	};
	
	QSharedPointer<IconList> icons();
	
	Status status()
	{
		return m_status;
	}
	
	MultiMCVersion version()
	{
		return m_version;
	}
	
	QSharedPointer<QNetworkAccessManager> qnam()
	{
		return m_qnam;
	}
	
	QSharedPointer<HttpMetaCache> metacache()
	{
		return m_metacache;
	}
	
	QSharedPointer<LWJGLVersionList> lwjgllist();
	
	QSharedPointer<ForgeVersionList> forgelist();
	
	QSharedPointer<MinecraftVersionList> minecraftlist();
private:
	void initGlobalSettings();
	
	void initHttpMetaCache();
	
	void initTranslations();
private:
	QSharedPointer<QTranslator> m_qt_translator;
	QSharedPointer<QTranslator> m_mmc_translator;
	QSharedPointer<SettingsObject> m_settings;
	QSharedPointer<InstanceList> m_instances;
	QSharedPointer<IconList> m_icons;
	QSharedPointer<QNetworkAccessManager> m_qnam;
	QSharedPointer<HttpMetaCache> m_metacache;
	QSharedPointer<LWJGLVersionList> m_lwjgllist;
	QSharedPointer<ForgeVersionList> m_forgelist;
	QSharedPointer<MinecraftVersionList> m_minecraftlist;
	
	Status m_status = MultiMC::Failed;
	MultiMCVersion m_version = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD};
};