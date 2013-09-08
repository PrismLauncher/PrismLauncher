#pragma once

#include <QApplication>
#include "MultiMCVersion.h"
#include "config.h"

class HttpMetaCache;
class SettingsObject;
class InstanceList;
class IconList;
class QNetworkAccessManager;

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
private:
	void initGlobalSettings();
	
	void initHttpMetaCache();
private:
	SettingsObject * m_settings = nullptr;
	InstanceList * m_instances = nullptr;
	IconList * m_icons = nullptr;
	QNetworkAccessManager * m_qnam = nullptr;
	HttpMetaCache * m_metacache = nullptr;
	Status m_status = MultiMC::Failed;
	MultiMCVersion m_version = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD};
};