#include "instancemodel.h"
#include <instance.h>
#include <QIcon>

InstanceModel::InstanceModel ( const InstanceList& instances, QObject *parent )
	: QAbstractListModel ( parent ), m_instances ( &instances )
{
	cachedIcon = QIcon(":/icons/multimc/scalable/apps/multimc.svg");
}

int InstanceModel::rowCount ( const QModelIndex& parent ) const
{
	Q_UNUSED ( parent );
	return m_instances->count();
}

QModelIndex InstanceModel::index ( int row, int column, const QModelIndex& parent ) const
{
	Q_UNUSED ( parent );
	if ( row < 0 || row >= m_instances->count() )
		return QModelIndex();
	return createIndex ( row, column, ( void* ) m_instances->at ( row ).data() );
}

QVariant InstanceModel::data ( const QModelIndex& index, int role ) const
{
	if ( !index.isValid() )
	{
		return QVariant();
	}
	Instance *pdata = static_cast<Instance*> ( index.internalPointer() );
	switch ( role )
	{
	case InstancePointerRole:
	{
		QVariant v = qVariantFromValue((void *) pdata);
		return v;
	}
	case Qt::DisplayRole:
	{
		return pdata->name();
	}
	case Qt::ToolTipRole:
	{
		return pdata->rootDir();
	}
	case Qt::DecorationRole:
	{
		// FIXME: replace with an icon cache
		return cachedIcon;
	}
	// for now.
	case KCategorizedSortFilterProxyModel::CategorySortRole:
	case KCategorizedSortFilterProxyModel::CategoryDisplayRole:
	{
		return "IT'S A GROUP";
	}
	default:
		break;
	}
	return QVariant();
}

Qt::ItemFlags InstanceModel::flags ( const QModelIndex& index ) const
{
	Qt::ItemFlags f;
	if ( index.isValid() )
	{
		f |= ( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
	}
	return f;
}

InstanceProxyModel::InstanceProxyModel ( QObject *parent )
	: KCategorizedSortFilterProxyModel ( parent )
{
	// disable since by default we are globally sorting by date:
	setCategorizedModel(true);
}

bool InstanceProxyModel::subSortLessThan (
 const QModelIndex& left, const QModelIndex& right ) const
{
	Instance *pdataLeft = static_cast<Instance*> ( left.internalPointer() );
	Instance *pdataRight = static_cast<Instance*> ( right.internalPointer() );
	//kDebug() << *pdataLeft << *pdataRight;
	return QString::localeAwareCompare(pdataLeft->name(), pdataRight->name()) < 0;
	//return pdataLeft->name() < pdataRight->name();
}

#include "instancemodel.moc"

