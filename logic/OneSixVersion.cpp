#include "OneSixVersion.h"
#include "OneSixLibrary.h"

QList<QSharedPointer<OneSixLibrary> > OneSixVersion::getActiveNormalLibs()
{
	QList<QSharedPointer<OneSixLibrary> > output;
	for ( auto lib: libraries )
	{
		if (lib->isActive() && !lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

QList<QSharedPointer<OneSixLibrary> > OneSixVersion::getActiveNativeLibs()
{
	QList<QSharedPointer<OneSixLibrary> > output;
	for ( auto lib: libraries )
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}


QVariant OneSixVersion::data(const QModelIndex& index, int role) const
{
	if(!index.isValid())
		return QVariant();
	
	int row = index.row();
	int column = index.column();
	
	if(row < 0 || row >= libraries.size())
		return QVariant();
	
	if(role == Qt::DisplayRole)
	{
		switch(column)
		{
			case 0:
				return libraries[row]->name();
			case 1:
				return libraries[row]->type();
			case 2:
				return libraries[row]->version();
			default:
				return QVariant();
		}
	}
	return QVariant();
}

Qt::ItemFlags OneSixVersion::flags(const QModelIndex& index) const
{
	if(!index.isValid())
		return Qt::NoItemFlags;
	int row = index.row();
	if(libraries[row]->isActive())
	{
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
	}
	else
	{
		return Qt::ItemNeverHasChildren;
	}
	//return QAbstractListModel::flags(index);
}


QVariant OneSixVersion::headerData ( int section, Qt::Orientation orientation, int role ) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();
	switch (section)
	{
	case 0:
		return QString("Name");
	case 1:
		return QString("Type");
	case 2:
		return QString("Version");
	default:
		return QString();
	}
}

int OneSixVersion::rowCount(const QModelIndex& parent) const
{
	return libraries.size();
}

int OneSixVersion::columnCount(const QModelIndex& parent) const
{
    return 3;
}
