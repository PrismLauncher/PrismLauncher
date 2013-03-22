#include "instancemodel.h"
#include <instance.h>
#include <QIcon>
#include "iconcache.h"

InstanceModel::InstanceModel ( const InstanceList& instances, QObject *parent )
	: QAbstractListModel ( parent ), m_instances ( &instances )
{
	currentInstancesNumber = m_instances->count();
	connect(m_instances,SIGNAL(instanceAdded(int)),this,SLOT(onInstanceAdded(int)));
	connect(m_instances,SIGNAL(instanceChanged(int)),this,SLOT(onInstanceChanged(int)));
	connect(m_instances,SIGNAL(invalidated()),this,SLOT(onInvalidated()));
}

void InstanceModel::onInstanceAdded ( int index )
{
	beginInsertRows(QModelIndex(), index, index);
	currentInstancesNumber ++;
	endInsertRows();
}

void InstanceModel::onInstanceChanged ( int index )
{
	QModelIndex mx = InstanceModel::index(index);
	dataChanged(mx,mx);
}

void InstanceModel::onInvalidated()
{
	beginResetModel();
	currentInstancesNumber = m_instances->count();
	endResetModel();
}


int InstanceModel::rowCount ( const QModelIndex& parent ) const
{
	Q_UNUSED ( parent );
	return m_instances->count();
}

QModelIndex InstanceModel::index ( int row, int column, const QModelIndex& parent ) const
{
	Q_UNUSED ( parent );
	if ( row < 0 || row >= currentInstancesNumber )
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
		IconCache * ic = IconCache::instance();
		// FIXME: replace with an icon cache/renderer
		/*
		QString path = ":/icons/instances/";
		path += pdata->iconKey();
		QIcon icon(path);
		*/
		QString key = pdata->iconKey();
		return ic->getIcon(key);
		//else return QIcon(":/icons/multimc/scalable/apps/multimc.svg");
	}
	// for now.
	case KCategorizedSortFilterProxyModel::CategorySortRole:
	case KCategorizedSortFilterProxyModel::CategoryDisplayRole:
	{
		return pdata->group();
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

