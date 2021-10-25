#include "InstanceProxyModel.h"
#include "Launcher.h"
#include <BaseInstance.h>
#include <icons/IconList.h>

InstanceProxyModel::InstanceProxyModel(QObject *parent) : GroupedProxyModel(parent)
{
    m_naturalSort.setNumericMode(true);
    m_naturalSort.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    // FIXME: use loaded translation as source of locale instead, hook this up to translation changes
    m_naturalSort.setLocale(QLocale::system());
}

QVariant InstanceProxyModel::data(const QModelIndex & index, int role) const
{
    QVariant data = QSortFilterProxyModel::data(index, role);
    if(role == Qt::DecorationRole)
    {
        return QVariant(LAUNCHER->icons()->getIcon(data.toString()));
    }
    return data;
}

bool InstanceProxyModel::subSortLessThan(const QModelIndex &left,
                                         const QModelIndex &right) const
{
    BaseInstance *pdataLeft = static_cast<BaseInstance *>(left.internalPointer());
    BaseInstance *pdataRight = static_cast<BaseInstance *>(right.internalPointer());
    QString sortMode = LAUNCHER->settings()->get("InstSortMode").toString();
    if (sortMode == "LastLaunch")
    {
        return pdataLeft->lastLaunch() > pdataRight->lastLaunch();
    }
    else
    {
        return m_naturalSort.compare(pdataLeft->name(), pdataRight->name()) < 0;
    }
}
