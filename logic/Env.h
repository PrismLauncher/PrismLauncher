#pragma once

#include <memory>
#include <QString>

class IconList;
class QNetworkAccessManager;
class HttpMetaCache;

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

protected:
	std::shared_ptr<QNetworkAccessManager> m_qnam;
	std::shared_ptr<HttpMetaCache> m_metacache;
	std::shared_ptr<IconList> m_icons;
};
