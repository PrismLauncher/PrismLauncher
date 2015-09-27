#pragma once

#include <QObject>
#include <resources/ResourceHandler.h>

class NetJob;

class WebResourceHandler : public QObject, public ResourceHandler
{
public:
	explicit WebResourceHandler(const QString &url);

private slots:
	void succeeded();
	void progress(qint64 current, qint64 total);

private:
	static QMap<QString, NetJob *> m_activeDownloads;

	QString m_url;

	void setResultFromFile(const QString &file);
};
