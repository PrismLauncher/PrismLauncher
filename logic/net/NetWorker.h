/*
        _.ooo-._
      .OOOP   _ '. 
     dOOOO   (_)  \
    OOOOOb         |
    OOOOOOb.       |
    OOOOOOOOb      |
     YOO(_)OOO    /
      'OOOOOY  _.'
        '""""''
*/
#pragma once

#include <QNetworkAccessManager>
#include <QUrl>

class NetWorker : public QObject
{
	Q_OBJECT
public:
	// for high level access to the sevices (preferred)
	static NetWorker &worker();
	// for low-level access to the network manager object
	static QNetworkAccessManager &qnam();
public:
	
private:
	explicit NetWorker ( QObject* parent = 0 );
	class Private;
	Private * d;
};