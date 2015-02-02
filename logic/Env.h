#pragma once

#include <memory>
#include <QString>
#include <QMap>

class IconList;
class QNetworkAccessManager;
class HttpMetaCache;
class BaseVersionList;
class BaseVersion;

#if defined(ENV)
	#undef ENV
#endif
#define ENV (Env::getInstance())

class Env
{
	friend class MultiMC;
private:
	Env();
public:
	static Env& getInstance();

	// call when Qt stuff is being torn down
	void destroy();

	std::shared_ptr<QNetworkAccessManager> qnam();

	std::shared_ptr<HttpMetaCache> metacache();

	std::shared_ptr<IconList> icons();

	/// init the cache. FIXME: possible future hook point
	void initHttpMetaCache(QString rootPath, QString staticDataPath);

	/// Updates the application proxy settings from the settings object.
	void updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password);

	/// get a version list by name
	std::shared_ptr<BaseVersionList> getVersionList(QString component);

	/// get a version by list name and version name
	std::shared_ptr<BaseVersion> getVersion(QString component, QString version);

	void registerVersionList(QString name, std::shared_ptr<BaseVersionList> vlist);
protected:
	std::shared_ptr<QNetworkAccessManager> m_qnam;
	std::shared_ptr<HttpMetaCache> m_metacache;
	std::shared_ptr<IconList> m_icons;
	QMap<QString, std::shared_ptr<BaseVersionList>> m_versionLists;
};
