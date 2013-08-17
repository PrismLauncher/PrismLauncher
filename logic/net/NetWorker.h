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
class NetWorker : public QNetworkAccessManager
{
	Q_OBJECT
public:
	static NetWorker &spawn();
};