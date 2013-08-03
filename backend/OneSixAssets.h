#pragma once
#include <QObject>
#include <QSharedPointer>

class Private;

class OneSixAssets : public QObject
{
	Q_OBJECT
signals:
	void failed();
	void finished();

public slots:
	void fetchFinished();
	void fetchStarted();
public:
	explicit OneSixAssets ( QObject* parent = 0 );
	void start();
	QSharedPointer<Private> d;
};
