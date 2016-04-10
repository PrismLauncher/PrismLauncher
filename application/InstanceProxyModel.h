#pragma once

#include "groupview/GroupedProxyModel.h"

/**
 * A proxy model that is responsible for sorting instances into groups
 */
class InstanceProxyModel : public GroupedProxyModel
{
public:
	explicit InstanceProxyModel(QObject *parent = 0);
	QVariant data(const QModelIndex & index, int role) const override;

protected:
	virtual bool subSortLessThan(const QModelIndex &left, const QModelIndex &right) const override;
};
