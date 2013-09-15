#pragma once

#include <QApplication>
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
	
	SettingsObject * settings()
	{
		return m_settings;
	};
	
	InstanceList * instances()
	{
		return m_instances;
	};
	
	IconList * icons();
	
	Status status()
	{
		return m_status;
	}
	
	MultiMCVersion version()
	{
		return m_version;
	}
	
	QNetworkAccessManager * qnam()
	{
		return m_qnam;
	}
	
	HttpMetaCache * metacache()
	{
		return m_metacache;
	}
	
	LWJGLVersionList * lwjgllist();
	
	ForgeVersionList * forgelist();
	
	MinecraftVersionList * minecraftlist();
private:
	void initGlobalSettings();
	
	void initHttpMetaCache();
	
	void initTranslations();
private:
	QTranslator * m_qt_translator = nullptr;
	QTranslator * m_mmc_translator = nullptr;
	SettingsObject * m_settings = nullptr;
	InstanceList * m_instances = nullptr;
	IconList * m_icons = nullptr;
	QNetworkAccessManager * m_qnam = nullptr;
	HttpMetaCache * m_metacache = nullptr;
	Status m_status = MultiMC::Failed;
	LWJGLVersionList * m_lwjgllist = nullptr;
	ForgeVersionList * m_forgelist = nullptr;
	MinecraftVersionList * m_minecraftlist = nullptr;
	
	MultiMCVersion m_version = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD};
};