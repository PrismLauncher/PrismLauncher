#pragma once

#include <logic/BaseInstance.h>

// Pseudo-plugin for FTB related things. Super derpy!
class FTBPlugin
{
public:
	static void initialize();
	static void loadInstances(QMap<QString, QString> &groupMap, QList<InstancePtr> &tempList);
};
