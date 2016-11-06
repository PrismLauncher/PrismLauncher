#pragma once

#include <memory>
#include "icons/IIconList.h"
#include <QString>
#include <QMap>

#include "multimc_logic_export.h"

#include "QObjectPtr.h"

class QNetworkAccessManager;
class HttpMetaCache;
class BaseVersionList;
class BaseVersion;
class WonkoIndex;

#if defined(ENV)
	#undef ENV
#endif
#define ENV (Env::getInstance())

class MULTIMC_LOGIC_EXPORT Env
{
	friend class MultiMC;
private:
	class Private;
	Env();
	~Env();
public:
	static Env& getInstance();

	QNetworkAccessManager &qnam() const;

	shared_qobject_ptr<HttpMetaCache> metacache();

	std::shared_ptr<IIconList> icons();

	/// init the cache. FIXME: possible future hook point
	void initHttpMetaCache();

	/// Updates the application proxy settings from the settings object.
	void updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password);

	/// get a version list by name
	std::shared_ptr<BaseVersionList> getVersionList(QString component);

	/// get a version by list name and version name
	std::shared_ptr<BaseVersion> getVersion(QString component, QString version);

	void registerVersionList(QString name, std::shared_ptr<BaseVersionList> vlist);

	void registerIconList(std::shared_ptr<IIconList> iconlist);

	shared_qobject_ptr<WonkoIndex> wonkoIndex();

	QString wonkoRootUrl() const;
	void setWonkoRootUrl(const QString &url);

protected:
	Private * d;
};
