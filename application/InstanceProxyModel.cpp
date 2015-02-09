#include "InstanceProxyModel.h"
#include "MultiMC.h"
#include <BaseInstance.h>

InstanceProxyModel::InstanceProxyModel(QObject *parent) : GroupedProxyModel(parent)
{
}

bool InstanceProxyModel::subSortLessThan(const QModelIndex &left,
										 const QModelIndex &right) const
{
	BaseInstance *pdataLeft = static_cast<BaseInstance *>(left.internalPointer());
	BaseInstance *pdataRight = static_cast<BaseInstance *>(right.internalPointer());
	QString sortMode = MMC->settings()->get("InstSortMode").toString();
	if (sortMode == "LastLaunch")
	{
		return pdataLeft->lastLaunch() > pdataRight->lastLaunch();
	}
	else
	{
		return QString::localeAwareCompare(pdataLeft->name(), pdataRight->name()) < 0;
	}
}
