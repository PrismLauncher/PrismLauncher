#include "InstanceProxyModel.h"
#include "MultiMC.h"
#include <BaseInstance.h>
#include <icons/IconList.h>

InstanceProxyModel::InstanceProxyModel(QObject *parent) : GroupedProxyModel(parent)
{
}

QVariant InstanceProxyModel::data(const QModelIndex & index, int role) const
{
	QVariant data = QSortFilterProxyModel::data(index, role);
	if(role == Qt::DecorationRole)
	{
		return QVariant(MMC->icons()->getIcon(data.toString()));
	}
	return data;
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
