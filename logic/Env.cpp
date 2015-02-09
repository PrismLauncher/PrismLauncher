#include "Env.h"
#include "net/HttpMetaCache.h"
#include "icons/IconList.h"
#include "BaseVersion.h"
#include "BaseVersionList.h"
#include <QDir>
#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QDebug>
#include "tasks/Task.h"
#include <QDebug>

/*
 * The *NEW* global rat nest of an object. Handle with care.
 */

Env::Env()
{
	m_qnam = std::make_shared<QNetworkAccessManager>();
}

void Env::destroy()
{
	m_metacache.reset();
	m_qnam.reset();
	m_icons.reset();
	m_versionLists.clear();
}

Env& Env::Env::getInstance()
{
	static Env instance;
	return instance;
}

std::shared_ptr< HttpMetaCache > Env::metacache()
{
	Q_ASSERT(m_metacache != nullptr);
	return m_metacache;
}

std::shared_ptr< QNetworkAccessManager > Env::qnam()
{
	return m_qnam;
}

std::shared_ptr<IconList> Env::icons()
{
	Q_ASSERT(m_icons != nullptr);
	return m_icons;
}
/*
class NullVersion : public BaseVersion
{
	Q_OBJECT
public:
	virtual QString name()
	{
		return "null";
	}
	virtual QString descriptor()
	{
		return "null";
	}
	virtual QString typeString() const
	{
		return "Null";
	}
};

class NullTask: public Task
{
	Q_OBJECT
public:
	virtual void executeTask()
	{
		emitFailed(tr("Nothing to do."));
	}
};

class NullVersionList: public BaseVersionList
{
	Q_OBJECT
public:
	virtual const BaseVersionPtr at(int i) const
	{
		return std::make_shared<NullVersion>();
	}
	virtual int count() const
	{
		return 0;
	};
	virtual Task* getLoadTask()
	{
		return new NullTask;
	}
	virtual bool isLoaded()
	{
		return false;
	}
	virtual void sort()
	{
	}
	virtual void updateListData(QList< BaseVersionPtr >)
	{
	}
};
*/

BaseVersionPtr Env::getVersion(QString component, QString version)
{
	auto list = getVersionList(component);
	if(!list)
	{
		return nullptr;
	}
	return list->findVersion(version);
}

std::shared_ptr< BaseVersionList > Env::getVersionList(QString component)
{
	auto iter = m_versionLists.find(component);
	if(iter != m_versionLists.end())
	{
		return *iter;
	}
	//return std::make_shared<NullVersionList>();
	return nullptr;
}

void Env::registerVersionList(QString name, std::shared_ptr< BaseVersionList > vlist)
{
	m_versionLists[name] = vlist;
}


void Env::initHttpMetaCache(QString rootPath, QString staticDataPath)
{
	m_metacache.reset(new HttpMetaCache("metacache"));
	m_metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
	m_metacache->addBase("asset_objects", QDir("assets/objects").absolutePath());
	m_metacache->addBase("versions", QDir("versions").absolutePath());
	m_metacache->addBase("libraries", QDir("libraries").absolutePath());
	m_metacache->addBase("minecraftforge", QDir("mods/minecraftforge").absolutePath());
	m_metacache->addBase("fmllibs", QDir("mods/minecraftforge/libs").absolutePath());
	m_metacache->addBase("liteloader", QDir("mods/liteloader").absolutePath());
	m_metacache->addBase("general", QDir("cache").absolutePath());
	m_metacache->addBase("skins", QDir("accounts/skins").absolutePath());
	m_metacache->addBase("root", QDir(rootPath).absolutePath());
	m_metacache->addBase("translations", QDir(staticDataPath + "/translations").absolutePath());
	m_metacache->Load();
}

void Env::updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password)
{
	// Set the application proxy settings.
	if (proxyTypeStr == "SOCKS5")
	{
		QNetworkProxy::setApplicationProxy(
			QNetworkProxy(QNetworkProxy::Socks5Proxy, addr, port, user, password));
	}
	else if (proxyTypeStr == "HTTP")
	{
		QNetworkProxy::setApplicationProxy(
			QNetworkProxy(QNetworkProxy::HttpProxy, addr, port, user, password));
	}
	else if (proxyTypeStr == "None")
	{
		// If we have no proxy set, set no proxy and return.
		QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
	}
	else
	{
		// If we have "Default" selected, set Qt to use the system proxy settings.
		QNetworkProxyFactory::setUseSystemConfiguration(true);
	}

	qDebug() << "Detecting proxy settings...";
	QNetworkProxy proxy = QNetworkProxy::applicationProxy();
	if (m_qnam.get())
		m_qnam->setProxy(proxy);
	QString proxyDesc;
	if (proxy.type() == QNetworkProxy::NoProxy)
	{
		qDebug() << "Using no proxy is an option!";
		return;
	}
	switch (proxy.type())
	{
	case QNetworkProxy::DefaultProxy:
		proxyDesc = "Default proxy: ";
		break;
	case QNetworkProxy::Socks5Proxy:
		proxyDesc = "Socks5 proxy: ";
		break;
	case QNetworkProxy::HttpProxy:
		proxyDesc = "HTTP proxy: ";
		break;
	case QNetworkProxy::HttpCachingProxy:
		proxyDesc = "HTTP caching: ";
		break;
	case QNetworkProxy::FtpCachingProxy:
		proxyDesc = "FTP caching: ";
		break;
	default:
		proxyDesc = "DERP proxy: ";
		break;
	}
	proxyDesc += QString("%3@%1:%2 pass %4")
					 .arg(proxy.hostName())
					 .arg(proxy.port())
					 .arg(proxy.user())
					 .arg(proxy.password());
	qDebug() << proxyDesc;
}

#include "Env.moc"