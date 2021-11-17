#pragma once

#include <memory>
#include "icons/IIconList.h"
#include <QString>
#include <QMap>

#include "QObjectPtr.h"

class QNetworkAccessManager;
class HttpMetaCache;
class BaseVersionList;
class BaseVersion;

namespace Meta
{
class Index;
}

#if defined(ENV)
    #undef ENV
#endif
#define ENV (Env::getInstance())


class Env
{
    friend class Launcher;
private:
    struct Private;
    Env();
    ~Env();
    static void dispose();
public:
    static Env& getInstance();

    QNetworkAccessManager &network() const;

    shared_qobject_ptr<HttpMetaCache> metacache();

    std::shared_ptr<IIconList> icons();

    /// init the cache. FIXME: possible future hook point
    void initHttpMetaCache();

    /// Updates the application proxy settings from the settings object.
    void updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password);

    void registerIconList(std::shared_ptr<IIconList> iconlist);

    shared_qobject_ptr<Meta::Index> metadataIndex();

    QString getJarsPath();
    void setJarsPath(const QString & path);

    bool isFeatureEnabled(const QString & featureName) const;
    void enableFeature(const QString & featureName, bool state = true);
    void getEnabledFeatures(QSet<QString> & features) const;
    void setEnabledFeatures(const QSet<QString> & features) const;

protected:
    Private * d;
};
